"""Exit 0 only for a device_probe_v1 manifest that is MEASURED and was written completely."""
import json
import sys

try:
    with open(sys.argv[1], encoding='utf-8') as f:
        manifest = json.load(f)
except (IndexError, OSError, ValueError) as error:
    print(f'unreadable manifest: {error}')
    sys.exit(1)

sys.exit(0 if manifest.get('status') == 'MEASURED' and manifest.get('manifest_write_success') is True else 1)
