from PIL import Image, ImageDraw
import serial.tools.list_ports
import struct
import serial
import time

from audio_manager import AudioManager


BAUD = 115200
CHUNK = 64
W, H = 80, 80


def checksum32(data: bytes) -> int:
    return sum(data) & 0xFFFFFFFF


def rgb888_to_565(r, g, b):
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)


class ESPComm:
    _sep = "<|>"
    debug = False
    
    def __init__(self, am: AudioManager, baud: int = BAUD):
        self._am: AudioManager = am
        self._baud = baud
        self._ser = self._try_get_serial()
        
        # create audio manager callbak
        self._am.set_on_change(self.update_volume)

        time.sleep(2)

    def _try_get_serial(self) -> None | serial.Serial:
        ser = None
        for p in serial.tools.list_ports.comports():
            if "CH343" in p.description:
                try:
                    ser = serial.Serial(p.device, self._baud, timeout=1)
                    ser.set_buffer_size(rx_size=65536, tx_size=65536)
                    
                    print(f"selected serial device: {p.device}, {p.description}")
                
                except serial.SerialException:
                    continue

        return ser

    def __send(self, msg: bytes) -> bool:
        """direct write with only basic checks"""
        # try getting new serial device
        if not self._ser:
            self._ser = self._try_get_serial()

        if self._ser:
            try:
                self._ser.write(msg)
                return True
            
            except serial.SerialException:
                self._ser = None
        
        return False

    def _send(self, msg: str) -> bool:
        # make sure message ends with newline
        if not msg.endswith("\n"):
            msg = msg + "\n"
        
        if self.debug:
            print(f"sending: {msg}", end="")
        
        return self.__send(msg.encode())

    def __recv(self) -> str | None:
        """receive one message"""
        # try getting new serial device
        if not self._ser:
            self._ser = self._try_get_serial()

        if self._ser:
            response = self._ser.readline().decode(errors="ignore").strip()

            if response:
                return response
        
        return None

    def _process_message(self, msg: str) -> None:
        """process a message from the ESP"""
        
        if self.debug:
            print(f"processing: \"{msg}\"")
        
        if msg.startswith("VOL"):
            split = msg.find(self._sep)
            
            if split != -1:
                vol = int(msg[split+len(self._sep):]) / 100
                self._am.set_volume(vol)

    def _receive_confirm(self) -> bool:
        """try to receive confirm message"""
        msg = self.__recv()
        
        if msg:
            if msg == "OK":
                return True
        
            self._process_message(msg)

        return False

    def update_volume(self, volume: float) -> None:
        """update volume (0-1)"""
        vol = round(volume*100, 0)

        # clamp volume
        vol = min(max(vol, 0), 100)
        
        msg = f"VOL{self._sep}{vol}\n"
        self._send(msg)

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

    def update_thumbnail(self, thumbnail, retry = 0) -> bool:
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
                pixel: tuple[int, ...] = thumbnail.getpixel((x, y))  # type: ignore
                r, g, b = pixel[0], pixel[1], pixel[2]
                color = rgb888_to_565(r, g, b)
                data += struct.pack("<H", color)  # little endian

        self.clear_buff()
        
        # send image
        cs = checksum32(data)

        start = time.perf_counter()

        self._send(f"IMG{self._sep}\n")
        if self._ser:
            self.__send(struct.pack("<I", len(data)))
            self.__send(struct.pack("<I", cs))
            self.__send(data)

            time.sleep(.1)
            
            # wait for response
            response = self._ser.readline().decode(errors="ignore").strip()
            if response:
                d = time.perf_counter() - start
                
                if self.debug:
                    print(f"image took {d:.2f}s ~ {len(data) / d:.2f} Bps, r: {response}")
    
                return response == "OK"

        # recursive retry
        if retry:
            return self.update_thumbnail(thumbnail=thumbnail, retry=retry-1)

        return False

    def cls(self) -> bool:
        """clear screen"""
        self.clear_buff()

        self._send(f"CLS{self._sep}\n")

        time.sleep(.1)
        
        # wait for response
        if self._ser:
            response = self._ser.readline().decode(errors="ignore").strip()
            if response:
                return response == "OK"
        
        return True

    def clear_buff(self) -> bool:
        """clear UART in buffer"""
        try:
            while self._ser and self._ser.in_waiting > 0:
                # get response
                response = self.__recv()

                if response:
                    # process response
                    self._process_message(response)

                time.sleep(.001)

        except serial.SerialException:
            self._ser = None
            return False

        return self._ser is not None
