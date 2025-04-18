# Manual Tests for AFISync

This document outlines the manual test procedure for AFISync. These tests are intentionally designed to be simple to execute, as more complex scenarios like delta patching are covered by automated integration tests. Testers should use their judgment to identify any unexpected behaviors or anomalies during the testing process.

## TC01: Mod Download

**Preconditions:**
-

**Steps:**
1. Delete a mod directory
2. Open AFISync

**Expected:**
Mod will download

---

## TC02: Download Limit

**Preconditions:**
-

**Steps:**
1. Open AFISync
2. Navigate to settings
3. Set a specific download speed limit (e.g., 500 KB/s)
4. Start downloading a large mod
5. Monitor the download speed
6. Change the limit during the download
7. Continue monitoring the download speed

**Expected:**
Download speed should not exceed the configured limit, changing the limit during download should take effect within reasonable time.

---

## TC03: TeamSpeak 3 plugin Install

**Preconditions:**
- TS3 is not running

**Steps:**
1. Delete TFAR TS3 plugin
2. Run recheck on TFAR

**Expected:**
Plugin will be installed

---

## TC04: Unable to Download Torrent

**Preconditions:**
- 

**Steps:**
1. Remove updateUrl from repositories.json 
2. Corrupt torrent url in repositories.json
3. Start AFISync

**Expected:**
Mod status should display an error indicating a failure to download the torrent file

---

## Notes for Testers

- Document any deviations from expected behavior with screenshots if possible
- Note the environment details (OS, hardware, network conditions) when conducting tests
- If a test fails, try to reproduce it at least once to confirm it's not an intermittent issue
