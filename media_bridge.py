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
import serial.tools.list_ports
from io import BytesIO
import typing as tp
import asyncio
import serial
import struct
import time

for p in serial.tools.list_ports.comports():
    print(p.device, p.description)

PORT = "COM15"
BAUD = 115200
CHUNK = 64
W, H = 80, 80


def rgb888_to_565(r, g, b):
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)


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
            media_info.get("artist"),
            media_info.get("title"),
            media_info.get("album_title"),
            position,
            duration,
            status
        )  # ignore: type


def checksum32(data: bytes) -> int:
    return sum(data) & 0xFFFFFFFF


class ESPComm:
    _sep = "<|>"
    
    def __init__(self, port: str = PORT, baud: int = BAUD):
        self.ser = serial.Serial(port, baud, timeout=1)
        self.ser.set_buffer_size(rx_size=65536, tx_size=65536)
        time.sleep(2)

    def _send(self, msg: str) -> None:
        self.ser.write(msg.encode())

    def update_info(self, artist: str, title: str, album: str) -> bool:
        """
        update song info
        :returns: True if successful, False otherwise
        """
        msg = f"INFO{self._sep}{artist}{self._sep}{title}{self._sep}{album}"
        self._send((msg + "\n"))
        
        return True
    
    def update_progress(self, position: float, duration: float, status: int) -> bool:
        """
        update progress bar
        """
        msg = f"PRG{self._sep}{position}{self._sep}{duration}{self._sep}{status}"
        self._send((msg + "\n"))

        return True

    def update_thumbnail(self, thumbnail) -> bool:
        """updates the thumbnail"""
        # convert to circle
        thumbnail = thumbnail.resize((H, W)).convert("RGB")

        mask = Image.new("L", (H, W), 0)
        draw = ImageDraw.Draw(mask)

        # draw.ellipse((0, 0, H, W), fill=255)
        draw.ellipse((0.5, 0.5, H - 0.5, W - 0.5), fill=255)
        
        bg = Image.new("RGB", (H, W), (0, 0, 0))  # or TFT background color
        thumbnail = Image.composite(thumbnail, bg, mask)
        
        data = bytearray()

        for y in range(H):
            for x in range(W):
                pixel: tuple[int, ...] = thumbnail.getpixel((x, y))
                r, g, b = pixel[0], pixel[1], pixel[2]
                color = rgb888_to_565(r, g, b)
                data += struct.pack("<H", color)  # little endian

        self.clear_buff()
        
        # send image
        cs = checksum32(data)

        start = time.perf_counter()

        self._send(f"IMG{self._sep}\n")
        self.ser.write(struct.pack("<I", len(data)))
        self.ser.write(struct.pack("<I", cs))
        self.ser.write(data)

        time.sleep(.1)
        
        # wait for response
        response = self.ser.readline().decode(errors="ignore").strip()
        if response:
            d = time.perf_counter() - start
            print(f"image took {d:.2f}s ~ {len(data) / d:.2f} Bps, r: {response}")
            return response == "OK"
        
        print(response)
        
        return True

    def cls(self) -> bool:
        """clear screen"""
        self.clear_buff()

        self._send(f"CLS{self._sep}\n")

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
            if response:
                print("> ", response)  # process commands here

            time.sleep(.001)


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

        # print awaiting messages
        comm.clear_buff()

        time.sleep(1)


if __name__ == "__main__":
    get_thumb_thread()
