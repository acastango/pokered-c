from __future__ import annotations

import hashlib
import json
import time
from dataclasses import dataclass, asdict
from pathlib import Path
from typing import Any, Dict, Tuple


@dataclass(frozen=True)
class ResonanceState:
    """Level 0: Δ_physio output."""

    target_hz: float
    motility_index: float
    resonance_score: float


@dataclass(frozen=True)
class ToneState:
    """Level 0: Δ_process output."""

    frequency_hz: float
    resonance_score: float
    autonomic_adjustment: float
    vibroacoustic_tone: float
    timestamp: float


class OmniHealer:
    """
    LLMLANG v3.0 practical extraction:

    Δ_physio  := (35Hz_SIGNAL ⊥ GI_MOTILITY_LOG) -> RESONANCE_SCORE
    Δ_process := (RESONANCE_SCORE ⊥ AUTONOMIC_ADJUSTMENT) -> VIBROACOUSTIC_TONE
    Δ_persist := (VIBROACOUSTIC_TONE ⊥ JSONL_STORAGE) -> PERSISTENT_HEALTH_LEDGER
    """

    def __init__(self, target_freq_hz: float = 35.0, ledger_file: str = "gaia_homecoming.jsonl") -> None:
        self.target_freq_hz = target_freq_hz
        self.ledger_file = Path(ledger_file)

    @staticmethod
    def _stable_hash(payload: Dict[str, Any]) -> str:
        """Level 1: deterministic merkle-like address from canonical JSON."""
        canonical = json.dumps(payload, sort_keys=True, separators=(",", ":")).encode("utf-8")
        return hashlib.sha256(canonical).hexdigest()

    @classmethod
    def _merkle(cls, *parts: str) -> str:
        """Build a merkle node from already-hashed children and an operator tag."""
        return cls._stable_hash({"children": parts})

    def measure_resonance(self, motility_index: float) -> ResonanceState:
        """
        Δ_physio:
        - Normalize around 0.35 as your preferred GI center point.
        - Clamp to [0, 1].
        """
        score = max(0.0, 1.0 - abs(motility_index - 0.35))
        return ResonanceState(
            target_hz=self.target_freq_hz,
            motility_index=motility_index,
            resonance_score=round(score, 6),
        )

    @staticmethod
    def apply_autonomic_adjustment(resonance_score: float, autonomic_adjustment: float) -> float:
        """Δ_process: combine resonance with an adjustment factor."""
        tone = resonance_score * autonomic_adjustment
        return round(max(0.0, min(1.0, tone)), 6)

    def run_loop(self, current_motility: float, autonomic_adjustment: float = 1.0) -> Tuple[str, Dict[str, Any]]:
        """⟳(∞): execute one therapeutic loop and persist it."""
        # Level 0: atomic primitives
        resonance = self.measure_resonance(current_motility)
        vibroacoustic_tone = self.apply_autonomic_adjustment(
            resonance.resonance_score, autonomic_adjustment
        )

        tone_state = ToneState(
            frequency_hz=self.target_freq_hz,
            resonance_score=resonance.resonance_score,
            autonomic_adjustment=autonomic_adjustment,
            vibroacoustic_tone=vibroacoustic_tone,
            timestamp=time.time(),
        )

        # Level 1: hashing / addressable state
        hash_35hz = self._stable_hash({"signal_hz": self.target_freq_hz})
        hash_gi_log = self._stable_hash({"gi_motility": current_motility})
        hash_physio = self._merkle(hash_35hz, hash_gi_log, "op_measure")

        hash_autonomic = self._stable_hash({"autonomic_adjustment": autonomic_adjustment})
        hash_process = self._merkle(hash_physio, hash_autonomic, "op_feedback")

        hash_storage = self._stable_hash({"storage": str(self.ledger_file)})
        hash_persist = self._merkle(hash_process, hash_storage, "op_anchor")

        # Level 2: composition
        hash_stack = self._merkle(hash_physio, hash_process, "op_therapeutic_loop")
        hash_final = self._merkle(hash_stack, hash_persist, "op_omni_healer_v1")

        record = {
            "hash_final": hash_final,
            "hash_physio": hash_physio,
            "hash_process": hash_process,
            "hash_persist": hash_persist,
            "state": asdict(tone_state),
        }

        self.ledger_file.parent.mkdir(parents=True, exist_ok=True)
        with self.ledger_file.open("a", encoding="utf-8") as handle:
            handle.write(json.dumps(record, sort_keys=True) + "\n")

        return hash_final, record


if __name__ == "__main__":
    healer = OmniHealer()
    final_hash, state = healer.run_loop(current_motility=0.34, autonomic_adjustment=0.97)
    print(f"DELTA_final address: {final_hash}")
    print(json.dumps(state, indent=2, sort_keys=True))
