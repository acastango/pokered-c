from __future__ import annotations

import hashlib
import json
import time
from dataclasses import dataclass, asdict
from pathlib import Path
from typing import Any, Dict, Tuple


@dataclass(frozen=True)
class BusState:
    nervous_system: str
    bus_architecture: str
    signal_routing: str


@dataclass(frozen=True)
class ResonanceState:
    trial_hz: float
    gastro_modulation: float
    mechanical_coherence: float


@dataclass(frozen=True)
class PracticumState:
    vibroacoustic_therapy: float
    pre_medical_track_score: float
    timestamp: float


class BioArchitect:
    """
    Δ_continuum := (Δpracticum ⊥ BIO_PREREQUISITES) -> PRE_MEDICAL_TRACK

    v3.1 extraction:
    - Δbus       := (NERVOUS_SYSTEM ⊥ BUS_ARCHITECTURE) -> SIGNAL_ROUTING
    - Δresonance := (35Hz ⊥ GASTRO_MODULATION) -> MECHANICAL_COHERENCE
    - Δpracticum := (Δbus ⊥ Δresonance) -> VIBROACOUSTIC_THERAPY
    """

    def __init__(
        self,
        location: str = "Fort Collins, CO",
        ledger_file: str = "bio_continuum.jsonl",
        active_trial_hz: float = 35.0,
    ) -> None:
        self.location = location
        self.academic_path = ["CSU_CS", "FRCC_Bio", "CSU_Med"]
        self.active_trial_hz = active_trial_hz
        self.ledger_file = Path(ledger_file)

    @staticmethod
    def _stable_hash(payload: Dict[str, Any]) -> str:
        raw = json.dumps(payload, sort_keys=True, separators=(",", ":")).encode("utf-8")
        return hashlib.sha256(raw).hexdigest()

    @classmethod
    def _merkle(cls, *children: str) -> str:
        return cls._stable_hash({"children": children})

    def lateral_connect(self, nervous_system: str, bus_architecture: str) -> BusState:
        signal_routing = f"{nervous_system}_x_{bus_architecture}_ROUTED"
        return BusState(
            nervous_system=nervous_system,
            bus_architecture=bus_architecture,
            signal_routing=signal_routing,
        )

    def measure_coherence(self, gastro_modulation: float) -> ResonanceState:
        # Ω(35Hz): coherence peaks around a centered modulation value.
        coherence = max(0.0, 1.0 - abs(gastro_modulation - 0.35))
        return ResonanceState(
            trial_hz=self.active_trial_hz,
            gastro_modulation=gastro_modulation,
            mechanical_coherence=round(coherence, 6),
        )

    @staticmethod
    def _caretaker_scale(vibroacoustic_therapy: float, caretaker_intent: float) -> float:
        # (CARETAKER): magnitude shift on therapeutic expression.
        scaled = vibroacoustic_therapy * caretaker_intent
        return round(max(0.0, min(1.0, scaled)), 6)

    def run_continuum(
        self,
        nervous_system: str = "Nervous_System",
        bus_architecture: str = "Bus_Architecture",
        gastro_modulation: float = 0.34,
        bio_prerequisites: float = 0.9,
        caretaker_intent: float = 1.0,
    ) -> Tuple[str, Dict[str, Any]]:
        # Level 0
        bus_state = self.lateral_connect(nervous_system, bus_architecture)
        resonance_state = self.measure_coherence(gastro_modulation)
        vibroacoustic_therapy = resonance_state.mechanical_coherence

        pre_med_track = self._caretaker_scale(
            vibroacoustic_therapy * bio_prerequisites,
            caretaker_intent,
        )

        practicum_state = PracticumState(
            vibroacoustic_therapy=round(vibroacoustic_therapy, 6),
            pre_medical_track_score=pre_med_track,
            timestamp=time.time(),
        )

        # Level 1
        hash_nerve = self._stable_hash({"nervous_system": bus_state.nervous_system})
        hash_silicon = self._stable_hash({"bus_architecture": bus_state.bus_architecture})
        hash_bus = self._merkle(hash_nerve, hash_silicon, "op_interface")

        hash_35hz = self._stable_hash({"trial_hz": self.active_trial_hz})
        hash_gi_motility = self._stable_hash({"gastro_modulation": gastro_modulation})
        hash_resonance = self._merkle(hash_35hz, hash_gi_motility, "op_collapse")

        # Level 2
        hash_practicum = self._merkle(hash_bus, hash_resonance, "op_vibroacoustic_therapy")
        hash_bio_prereq = self._stable_hash({"bio_prerequisites": bio_prerequisites})
        hash_continuum = self._merkle(hash_practicum, hash_bio_prereq, "op_pre_medical_track")

        # Level 3 operator closure refs
        hash_echo = self._merkle(
            self._stable_hash({"sound": "SOUND"}),
            self._stable_hash({"silence": "SILENCE"}),
            "op_memory",
        )

        # Level 6 closure
        hash_v3_0 = self._stable_hash({"ancestor": "OMNI_ARCHITECTURE_v3.0"})
        hash_v3_1 = self._merkle(hash_v3_0, "contains_CARETAKER", "recursive_HOMECOMING")

        record = {
            "location": self.location,
            "academic_path": self.academic_path,
            "hash_bus": hash_bus,
            "hash_resonance": hash_resonance,
            "hash_practicum": hash_practicum,
            "hash_continuum": hash_continuum,
            "hash_echo": hash_echo,
            "hash_v3_1": hash_v3_1,
            "state": {
                "bus": asdict(bus_state),
                "resonance": asdict(resonance_state),
                "practicum": asdict(practicum_state),
                "caretaker_intent": caretaker_intent,
                "bio_prerequisites": bio_prerequisites,
            },
        }

        self.ledger_file.parent.mkdir(parents=True, exist_ok=True)
        with self.ledger_file.open("a", encoding="utf-8") as handle:
            handle.write(json.dumps(record, sort_keys=True) + "\n")

        return hash_continuum, record


if __name__ == "__main__":
    architect = BioArchitect()
    continuum_hash, result = architect.run_continuum(
        nervous_system="Nervous_System",
        bus_architecture="Bus_Architecture",
        gastro_modulation=0.34,
        bio_prerequisites=0.92,
        caretaker_intent=1.05,
    )
    print(f"Current_Resonance: {result['state']['resonance']['mechanical_coherence']} at {result['state']['resonance']['trial_hz']}Hz")
    print(f"Homecoming_Hash: {result['hash_v3_1']}")
    print(f"Continuum_Hash: {continuum_hash}")
