from pycaw.pycaw import AudioUtilities
import typing as tp
import keyboard


class AudioManager:
    def __init__(self) -> None:
        self._volume = 0
        self._change_volume_callback: tp.Callable[[float], tp.Any] | None = None

    def set_on_change(self, cb: tp.Callable[[float], tp.Any]) -> None:
        """set on volume change callback"""
        self._change_volume_callback = cb
    
    def get_volume(self) -> float:
        """get current master volume from 0-1"""
        return self._volume

    def set_volume(self, value: float) -> bool:
        """
        set master volume between 0-1
        
        :param value: volume to set
        :returns: true if success
        """
        # clamp value
        volume = min(max(value, 0), 1)
        
        speakers = AudioUtilities.GetSpeakers()
        
        if speakers:
            self._volume = volume
            speakers.EndpointVolume.SetMasterVolumeLevelScalar(volume, None)
            return True
        
        return False

    def update(self) -> None:
        """update callbacks"""
        # update volume
        speakers = AudioUtilities.GetSpeakers()
        
        # try to get master volume
        if speakers:
            vol = speakers.EndpointVolume.GetMasterVolumeLevelScalar()

            # update callback
            if round(vol * 100, 0) != round(self._volume * 100, 0):
                if self._change_volume_callback:
                    self._change_volume_callback(vol)
            
            self._volume = vol

    def toggle_pause(self) -> None:
        """pause / unpause media"""
        keyboard.send("play/pause media")

    def skip(self) -> None:
        """skip track"""
        keyboard.send("next track")
    
    def skip_prev(self) -> None:
        """go to previous track"""
        keyboard.send("previous track")
