"""
media_bridge.py
29.04.2026

acts as a bridge to provide the ESP gadget with ICON / song data

Author:
Nilusink
"""
from winrt.windows.media.control import GlobalSystemMediaTransportControlsSessionManager as MediaManager
from winrt.windows.storage.streams import DataReader, Buffer, InputStreamOptions
from io import BytesIO
from PIL import Image
import pygame as pg
import typing as tp
import threading
import asyncio
import time
import math




async def get_thumbnail() -> None | tuple[tp.Any, str, str, str]:
    """:returns: thumbnail, artist, track title, album title"""
    sessions = await MediaManager.request_async()
    current_session = sessions.get_current_session()

    if not current_session:
        return None

    playback_info = current_session.get_playback_info()
    status = playback_info.playback_status
    properties = await current_session.try_get_media_properties_async()

    media_info = {
        song_attr: properties.__getattribute__(song_attr)
        for song_attr in dir(properties)
        if song_attr and song_attr[0] != '_'
    }

    # --- playback timeline ---
    timeline = current_session.get_timeline_properties()

    position = timeline.position.total_seconds() if timeline.position else 0
    duration = timeline.end_time.total_seconds() if timeline.end_time else 0

    # --- thumbnail ---
    if media_info.get('thumbnail'):
        thumb_stream_ref = media_info['thumbnail']

        readable_stream = await thumb_stream_ref.open_read_async()

        size = readable_stream.size
        thumb_read_buffer = Buffer(size)

        await readable_stream.read_async(thumb_read_buffer, size, 0)

        buffer_reader = DataReader.from_buffer(thumb_read_buffer)

        byte_buffer = bytearray(size)
        buffer_reader.read_bytes(byte_buffer)

        binary = BytesIO()
        binary.write(byte_buffer)
        binary.seek(0)

        img = Image.open(binary)

        return (
            img,
            media_info.get("artist"),
            media_info.get("title"),
            media_info.get("album_title"),
            position,
            duration,
            status
        )


def get_pg_surf(image: Image) -> pg.Surface:
    """get surface from thumbnail"""
    mode = image.mode
    size = image.size
    data = image.tobytes()

    return pg.image.fromstring(data, size, mode)


def circular_crop(surface, radius):
    size = surface.get_size()

    # create same-size alpha mask
    mask = pg.Surface(size, pg.SRCALPHA)
    mask.fill((0, 0, 0, 0))

    pg.draw.circle(mask, (255, 255, 255, 255), (radius, radius), radius)

    # copy original surface
    result = surface.copy()
    result.blit(mask, (0, 0), special_flags=pg.BLEND_RGBA_MULT)

    return result


def darken(surface, factor=0.5):
    overlay = pg.Surface(surface.get_size())
    overlay.fill((0, 0, 0))
    overlay.set_alpha(255*factor)

    surface.blit(overlay, (0, 0))
    return surface


def fast_blur(surface, scale=0.25):
    w, h = surface.get_size()

    small = pg.transform.smoothscale(surface, (int(w * scale), int(h * scale)))
    return pg.transform.smoothscale(small, (w, h))


CENTER_SIZE = 80, 80
CENTER_OFFSET = 120-(CENTER_SIZE[0]/2), 120-(CENTER_SIZE[1]/2)
thumbnail, artist, title, a_title, pos, dur, status = None, None, None, None, 0, 0, None
last_update = time.time()
UPDATE_INTERVAL = 2.0  # seconds
HALO_SIZE = 1
running = True

loop = asyncio.new_event_loop()
asyncio.set_event_loop(loop)

pg.init()

def get_thumb_thread():
    global thumbnail, artist, title, a_title, running, pos, dur, last_update, status

    while running:
        loop = asyncio.new_event_loop()
        asyncio.set_event_loop(loop)
        res = loop.run_until_complete(get_thumbnail())
        if res:
            thumbnail, artist, title, a_title, new_pos, dur, status = res

            if new_pos != pos:
                pos = new_pos
                last_update = time.time()
    
        time.sleep(1)

threading.Thread(target=get_thumb_thread, daemon=True).start()

screen = pg.display.set_mode((240, 240))
pg.display.set_caption("Now Playing")

_fonts = [
    pg.font.SysFont(None, 24),
    pg.font.SysFont(None, 20),
    pg.font.SysFont(None, 16),
    pg.font.SysFont(None, 12),
    pg.font.SysFont(None, 8),
]

def render_text(
        text: str,
        color: tuple[float, float, float] | tuple[float, float, float, float],
        starting_size: int = 0,
        max_len: int = 200
        ) -> pg.Surface:
    """dynamically render a text based on size"""
    for i in range(starting_size, len(_fonts)):
        _rendered = _fonts[i].render(text, True, color)

        if _rendered.get_size()[0] < max_len:
            return _rendered
    
    else:
        return _rendered


def render_prog_bar(t: float, length: int, height: int = 10, status=4) -> pg.Surface:
    """sine-wave progress bar with time offset"""

    surf = pg.Surface((length, height), pg.SRCALPHA)
    center_y = height / 2

    if status != 4:
        pg.draw.line(surf, (255, 255, 255), (0, center_y), (length * t, center_y))
        pg.draw.line(surf, (128, 128, 128), (length * t, center_y), (length, center_y))
        return surf

    amp = height * .20  # wave amplitude
    freq = 4             # number of waves along bar
    speed = 1.0          # animation speed

    phase = time.time() * speed

    # progress split point
    split = center_y + int((length - height) * t)

    points_left = []
    points_right = []
    point_pos = 0, 0

    for x in range(int(center_y), int(length - center_y)):
        y = center_y + math.sin((x / length) * freq * 2 * math.pi + phase) * amp

        if x <= split:
            points_left.append((x, y))

        else:
            points_right.append((x, y))

    point_pos = split,  center_y + math.sin((split / length) * freq * 2 * math.pi + phase) * amp


    if len(points_left) > 1:
        pg.draw.lines(surf, (255, 255, 255), False, points_left, 2)

    if len(points_right) > 1:
        pg.draw.lines(surf, (128, 128, 128), False, points_right, 2)

    if point_pos[1] > 0:
        pg.draw.circle(surf, (255, 255, 255), point_pos, height / 3)

    return surf


title_surface = None
artist_surface = None
a_title_surface = None

running = True
while running:
    now = time.time()
    screen.fill((50, 50, 50))

    for event in pg.event.get():
        if event.type == pg.QUIT:
            running = False

    if thumbnail:
        title_surface = render_text(title, (255, 255, 255), max_len=160)
        artist_surface = render_text(artist,(200, 200, 200), 1)
        a_title_surface = render_text(a_title, (180, 180, 180), 1)

        # apply media stuff
        surface = get_pg_surf(thumbnail.resize((240, 240)))
        surface = pg.transform.smoothscale(surface, (240, 240))
        surface = darken(fast_blur(circular_crop(surface, 120),), .6)

        surface2 = get_pg_surf(thumbnail.resize(CENTER_SIZE))
        surface2 = circular_crop(surface2, CENTER_SIZE[0]/2)

        surface1 = pg.Surface((CENTER_SIZE[0] + 2*HALO_SIZE, CENTER_SIZE[1] + 2*HALO_SIZE), pg.SRCALPHA)
        pg.draw.circle(surface1, (150, 150, 150, 40), (CENTER_SIZE[0]/2 + HALO_SIZE, CENTER_SIZE[1]/2 + HALO_SIZE), CENTER_SIZE[1]/2 + HALO_SIZE)

        screen.blit(surface, (0, 0))
        screen.blit(surface1, (CENTER_OFFSET[0]-HALO_SIZE, CENTER_OFFSET[1]-HALO_SIZE))
        screen.blit(surface2, CENTER_OFFSET)

        # draw texts
        screen.blit(title_surface, ((240-title_surface.get_size()[0])/2, 30))
        screen.blit(a_title_surface, ((240-a_title_surface.get_size()[0])/2, 50))
        screen.blit(artist_surface, ((240-artist_surface.get_size()[0])/2, 170))

        if status == 4:
            t = min((pos + now-last_update) / dur, 1)
        
        else:
            t = pos / dur

        screen.blit(render_prog_bar(t, 160, 20, status=status), (40, 195))

    pg.display.flip()

pg.quit()
running = False