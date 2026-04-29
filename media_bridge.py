"""
media_bridge.py
29.04.2026

acts as a bridge to provide the ESP gadget with ICON / song data

Author:
Nilusink
"""
from winrt.windows.media.control import GlobalSystemMediaTransportControlsSessionManager as MediaManager
from winrt.windows.storage.streams import DataReader, Buffer, InputStreamOptions
from PIL import Image, ImageDraw
from io import BytesIO
import typing as tp
import asyncio
import serial
import struct
import time


PORT = "COM13"        # change to your port (Linux: /dev/ttyUSB0)
BAUD = 115200
W, H = 80, 80


def rgb888_to_565(r, g, b):
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)


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


class ESPComm:
    _sep = "<|>"
    
    def __init__(self, port: str = PORT, baud: int = BAUD):
        self.ser = serial.Serial(port, baud, timeout=1)
        time.sleep(2)

    def update_info(self, artist: str, title: str, album: str) -> bool:
        """
        update song info
        :returns: True if successful, False otherwise
        """
        msg = f"INFO{self._sep}{artist}{self._sep}{title}{self._sep}{album}"
        self.ser.write((msg + "\n").encode())
        
        return True
    
    def update_progress(self, position: float, duration: float, status: int) -> bool:
        """
        update progress bar
        """
        msg = f"PRG{self._sep}{position}{self._sep}{duration}{self._sep}{status}"
        self.ser.write((msg + "\n").encode())

        return True

    def update_thumbnail(self, thumbnail) -> bool:
        """updates the thumbnail"""
        # clear buff
        self.clear_buff()
        
        # convert to circle
        thumbnail = thumbnail.resize((H, W)).convert("RGB")

        mask = Image.new("L", (H, W), 0)
        draw = ImageDraw.Draw(mask)

        draw.ellipse((0, 0, H - 1, W - 1), fill=255)
        
        bg = Image.new("RGB", (H, W), (0, 0, 0))  # or TFT background color
        thumbnail = Image.composite(thumbnail, bg, mask)
        
        data = bytearray()

        for y in range(H):
            for x in range(W):
                pixel = thumbnail.getpixel((x, y))
                r, g, b = pixel[0], pixel[1], pixel[2]
                color = rgb888_to_565(r, g, b)
                data += struct.pack("<H", color)  # big endian
        
        # send image
        self.ser.write(f"IMG{self._sep}\n".encode())
        self.ser.write(data)

        time.sleep(.1)
        
        # wait for response
        response = self.ser.readline().decode(errors="ignore").strip()
        if response:
            return response == "OK"
        
        return True

    def cls(self) -> None:
        """clear screen"""
        self.clear_buff()

        self.ser.write(f"CLS{self._sep}\n".encode())

        time.sleep(.1)
        
        # wait for response
        response = self.ser.readline().decode(errors="ignore").strip()
        if response:
            return response == "OK"
        
        return True

    def clear_buff(self):
        """clear UART in buffer"""
        while self.ser.in_waiting > 0:
            response = self.ser.readline().decode(errors="ignore").strip()
            # if response:
            #     print("response: ", response)  # process commands here

            time.sleep(.1)


def get_thumb_thread():
    comm = ESPComm()
    time.sleep(2)

    pos = -1
    
    last_title = ""
    while True:
        loop = asyncio.new_event_loop()
        asyncio.set_event_loop(loop)
        res = loop.run_until_complete(get_thumbnail())
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

                # update text again for it to be visible
                comm.update_info(artist, title, a_title)
    
        # print awaiting messages
        comm.clear_buff()

        time.sleep(1)


if __name__ == "__main__":
    get_thumb_thread()
