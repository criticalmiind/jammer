import re
import os

cpp_file = "menu_manager.cpp"
h_file = "menu_manager.h"

# Clean header
with open(h_file, "r", encoding="utf-8") as f:
    text = f.read()

# Remove STATE_BLE_* entirely
text = re.sub(r'STATE_BLE_[A-Z_]+,?\s*', '', text)
with open(h_file, "w", encoding="utf-8") as f:
    f.write(text)

# Clean cpp
with open(cpp_file, "r", encoding="utf-8") as f:
    text = f.read()

# 1. Remove BLE includes
text = re.sub(r'#include "ble_scanner\.h"\n', '', text)
text = re.sub(r'#include "ble_attacks\.h"\n', '', text)

# 2. Remove BLE results cache block lines 37-40
text = re.sub(r'// ── BLE scan results cache.*?\n.*?_bleResults\[MAX_BLE_RESULTS\];.*?\n.*?_bleCount = 0;.*?\n', '', text, flags=re.DOTALL)

# 3. Remove "BLE Scanner" string from main items array
text = text.replace('  "BLE Scanner",\n', '')
text = re.sub(r'static const uint8_t _mainCount = 7;', 'static const uint8_t _mainCount = 6;', text)

# 4. Remove BLE submenu block
text = re.sub(r'// ── BLE Sub-Menu ──.*?(?=// ──)', '', text, flags=re.DOTALL)

# 5. Remove BLE Flood from attack items
text = text.replace('  "BLE Flood",\n', '')
text = re.sub(r'static const uint8_t _attackMenuCount = 3;', 'static const uint8_t _attackMenuCount = 2;', text)

# 6. Remove BLE logic from main handler
# It's at case 1: _setState(STATE_BLE_MENU); break; 
text = re.sub(r'case 1:\s*_setState\(STATE_BLE_MENU\);\s*break;\n\s*', '', text)
# re-number cases
text = text.replace('case 2: _setState(STATE_NRF_MENU);', 'case 1: _setState(STATE_NRF_MENU);')
text = text.replace('case 3: _setCursorBounds(_attackMenuCount - 1);', 'case 2: _setCursorBounds(_attackMenuCount - 1);')
text = text.replace('case 3: _setState(STATE_ATTACK_MENU);', 'case 2: _setState(STATE_ATTACK_MENU);')
text = text.replace('case 4:', 'case 3:')
text = text.replace('case 5:', 'case 4:')
text = text.replace('case 6:', 'case 5:')

# 7. Remove BLE handlers (STATE_BLE_MENU, STATE_BLE_RESULTS)
text = re.sub(r'// ── BLE Menu Handler ──.*?(?=// ──)', '', text, flags=re.DOTALL)

# 8. Remove BLE Flood attack logic
text = re.sub(r'case 1:\s*_setState\(STATE_BLE_FLOOD_CONFIRM\);\s*break;\n\s*', '', text)
text = text.replace('case 2: _setState(STATE_DEAUTH_MENU);', 'case 1: _setState(STATE_DEAUTH_MENU);')

# 9. Strip out BLE scanning blocking state
text = re.sub(r'// ── BLE scan \(blocking but handled in state entry\).*?\n.*?if \(_state == STATE_BLE_SCANNING\).*?_setState\(STATE_BLE_RESULTS\);\s*}\n\n', '', text, flags=re.DOTALL)

# 10. Strip out BLE flood running state updates
text = re.sub(r'if \(_state == STATE_BLE_FLOOD_RUNNING\).*?ble_attacks_update\(\);\n\s*}\n\n', '', text, flags=re.DOTALL)
text = re.sub(r'if \(_state == STATE_BLE_FLOOD_RUNNING\).*?ble_attacks_stop\(\);\n\s*return;\n\s*}\n', '', text, flags=re.DOTALL)

# 11. Remove from switch tables
text = re.sub(r'case STATE_BLE_MENU:.*?break;', '', text, flags=re.DOTALL)
text = re.sub(r'case STATE_BLE_RESULTS:.*?break;', '', text, flags=re.DOTALL)
text = re.sub(r'case STATE_BLE_SCANNING:.*?break;', '', text, flags=re.DOTALL)

with open(cpp_file, "w", encoding="utf-8") as f:
    f.write(text)
