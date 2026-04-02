#!/usr/bin/env python3
"""Bug reporter for pokered.
Usage: python bug_report.py <screenshot_path>
Prompts for bug type + description, saves a markdown report to bugs/inbox/.
"""
import sys
import os
import datetime

TYPES = ["visual", "gameplay", "crash", "audio", "collision", "text", "other"]

def main():
    screenshot = sys.argv[1] if len(sys.argv) > 1 else None

    print()
    print("=== Pokered Bug Reporter ===")
    if screenshot:
        print(f"Screenshot: {screenshot}")
    print()

    # Bug type
    print("Bug types: " + ", ".join(TYPES))
    bug_type = input("Type: ").strip().lower() or "other"
    if bug_type not in TYPES:
        bug_type = "other"

    # Description
    description = input("Description: ").strip()
    if not description:
        print("No description — aborting.")
        input("Press Enter to close...")
        return

    # Write report
    ts = datetime.datetime.now()
    ts_str = ts.strftime("%Y%m%d_%H%M%S")
    script_dir = os.path.dirname(os.path.abspath(__file__))
    inbox = os.path.join(script_dir, "..", "bugs", "inbox")
    os.makedirs(inbox, exist_ok=True)

    report_name = f"{ts_str}_{bug_type}.md"
    report_path = os.path.join(inbox, report_name)

    with open(report_path, "w", encoding="utf-8") as f:
        f.write(f"# Bug: {bug_type}\n\n")
        f.write(f"**Date:** {ts.isoformat()}\n\n")
        f.write(f"**Type:** {bug_type}\n\n")
        f.write(f"**Description:**\n{description}\n")
        if screenshot:
            abs_ss = os.path.abspath(screenshot)
            f.write(f"\n**Screenshot:** {abs_ss}\n")

    print()
    print(f"Report saved: {os.path.abspath(report_path)}")
    input("Press Enter to close...")

if __name__ == "__main__":
    main()
