from pycaw.pycaw import AudioUtilities
import typing as tp


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
            if vol != self._volume:
                if self._change_volume_callback:
                    self._change_volume_callback(vol)
            
            self._volume = vol
