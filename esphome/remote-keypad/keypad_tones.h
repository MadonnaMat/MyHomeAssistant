#pragma once

namespace keypad_tones {

// Returns the RTTTL note fragment for a keypad key (one of "123A456B789C*0#D").
// Varied pitch & rhythm combinations, played at octave 6 (base), notes
// within the 4-7 range.
inline const char *note_for_key(char key) {
  switch (key) {
    case '1': return "16c7,16d7,16c6";   // High-High-Low
    case '2': return "16d7,16b6,16d7";   // High-Low-High
    case '3': return "32c6,32c7,32d7";   // Triple-Rising
    case 'A': return "32d7,32c7,32c6";   // Triple-Falling

    case '4': return "16c6,32c7,16c6";   // Punchy Sandwich
    case '5': return "16b6,16c7,32d7";   // Climbing
    case '6': return "32d7,16c7,16b6";   // Dropping
    case 'B': return "16c7,32d7,16c7";   // Bounce

    case '7': return "16c6,16b6,16c7";   // Mixed-Mid
    case '8': return "32c7,16d7,32c7";   // Center-Peak
    case '9': return "16d7,32b6,16d7";   // Wide-Swing
    case 'C': return "32b6,16c6,32b6";   // Tight-Sandwich

    case '*': return "16c7,16c7,32d7";   // Heavy-End
    case '0': return "32c6,32c6,16b6";   // Light-Start
    case '#': return "16d7,32c7,32c7";   // Heavy-Start
    case 'D': return "32b6,16d7,32b6";   // Sharp-Bounce
    default: return "";
  }
}

}  // namespace keypad_tones
