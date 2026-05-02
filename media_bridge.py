"""
media_bridge.py
29.04.2026

acts as a bridge to provide the ESP gadget with ICON / song data

Author:
Nilusink
"""
from winrt.windows.media.control import GlobalSystemMediaTransportControlsSessionManager as MediaManager
from winrt.windows.storage.streams import DataReader, Buffer, InputStreamOptions
from threading import Thread
from queue import Queue
from io import BytesIO
from PIL import Image
import typing as tp
import asyncio
import time

from audio_manager import AudioManager
from esp_comm import ESPComm


async def get_thumbnail() -> None | tuple[tp.Any, str, str, str, float, float, int]:
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
            media_info.get("artist", "unkown"),
            media_info.get("title", "unkown"),
            media_info.get("album_title", "unkown"),
            position,
            duration,
            status
        )


running = True
def get_thumb_thread(q: Queue):
    """start thumbnail thread (because pycaw and winrt conflict in same thread)"""
    global running
    
    while running:
        loop = asyncio.new_event_loop()
        asyncio.set_event_loop(loop)

        # get results
        res = loop.run_until_complete(get_thumbnail())
        
        # wait for main thread to get result
        while not q.empty():
            if not running:
                return
            
            time.sleep(.01)
        
        # transmit results to main thread
        q.put(res)


def main():
    global running
    am = AudioManager()
    comm = ESPComm(am)

    pos = -1
    
    results_queue = Queue()

    # start thumbnail thread
    Thread(target=get_thumb_thread, args=(results_queue,)).start()
    
    last_title = ""
    try:
        while True:
            # update media info if available
            if not results_queue.empty():
                res = results_queue.get()

                if res:
                    thumbnail, artist, title, a_title, new_pos, dur, status = res

                    if new_pos != pos:
                        pos = new_pos
                        comm.update_progress(pos, dur, status)
            
                    if title != last_title:
                        last_title = title

                        # draw info first
                        comm.cls()

                        # continue
                        comm.update_progress(pos, dur, status)
                        comm.update_info(artist, title, a_title)

                        # draw thumbnail
                        comm.update_thumbnail(thumbnail)

            # print awaiting messages
            if not comm.clear_buff():
                last_title = ""

            # update Audio callbacks
            am.update()
            
            time.sleep(.1)

    except KeyboardInterrupt:
        print("closing...")
    
    finally:
        running = False


if __name__ == "__main__":
    main()
