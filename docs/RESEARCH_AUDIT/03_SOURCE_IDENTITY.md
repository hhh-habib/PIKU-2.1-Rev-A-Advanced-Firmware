# Source identity and results matrix

The CSV contains 24 qualified claims. INDEPENDENTLY_SUPPORTED means this review checked implementation/file identity or the cited record directly; a checked run record is not a new execution or independent robot replication. AUTHOR_REPORTED identifies owner-confirmed physical observations/counts. CALCULATED identifies arithmetic, with applicability qualified: the old Wilson calculation is explicitly inapplicable. UNRESOLVED identifies missing identity/method/publication details.

## Source and revision identity

| Identity | Establishes | Does not establish |
|---|---|---|
| September hardware baseline v2.1-revA-final, commit 8f6f2dc9a9aa507493003fda701efd953d3b13cd | Historical hardware/configuration and dated functional observations | Nov 7 source or five comparator binaries |
| Oct 1 V3.2 executable 41281d8325d88c70e074e008a2550a1164102132 | Owner-reported supervised functional PASS | The 500-encounter benchmark or physical timing distributions |
| Current HEAD fa375a7bf739fc306060048b500c2acf413db7a7 | Historical checked-out commit | Dirty working-tree identity or current flash image |
| Oct 18 PAPER_READY fingerprint f979ca73108a358b06e3f2f733f97bcb9ecde6d7a69f7323cd5c9ec3251f34b6 | Dated 207 pass/11 skip; RAM 58996; flash 1059977; reported binary hash a2fbf661719c39734656a2a246457b9c5830a08668840c69d182179aa3c16aaf | Nov 7 result or robot readback |
| Nov 7 change manifest/continuity verification | All 19 recorded changed/added hashes match; dated 266 pass/11 skip of 277,13 native policies, RAM 61188, flash 1061465 | A new run here, raw run logs recovered here, or a current flash identity |
| Current nonprivate production snapshot | 49 files; fingerprint `331a3f4c47580255a0d7a9635d67d6ec406d4a84d7d7e43a63c17d6f1e888164` | Private configuration or complete compiled image |
| Available local software ZIP | SHA256 d52086051862dcef084689cd61de773d016f82bd6087f58c0ab4e6fe6235754f; all selected production files match current; contains Nov 7 docs plus preserved Oct 18 report | Identity of audit-named `...(20261102-215950).zip`, not located in bounded project/attachment/Downloads-root search |
| B0-B4 experimental variants | Owner-confirmed physical configurations and aggregate descriptions | Exact tested source/images/dates unresolved |

Fingerprint recipe: sort slash-relative src/include paths plus platformio.ini, excluding Secrets.h; SHA256 UTF8 records `relpath + NUL + file_sha256 + LF`. This is a different documented recipe from historical PAPER_READY, so differing fingerprint strings are not a direct comparison. Private configuration was excluded. Manifests snapshot hashes, not a shareable complete firmware build.

No tests/builds/flashes were run. Existing .pio metadata/dependencies do not supply the missing Nov 7 raw regression outputs or firmware image. The dated verification document and preserved mission verification are documentary support. A newly rebuilt binary would not authenticate a historical trial binary.

## Electrical precision

Documented 47k upper/100k lower gives 3.40136 V at 5 V and 3.53741 V at 5.2 V. Assuming fixed 5.2 V and +/-5% tolerance, upper 44.65k/lower 105k gives 3.64851 V. Actual tolerance, orientation, rail/signal peaks and board voltages were not measured here.

The [ESP32-S3 datasheet](https://documentation.espressif.com/esp32_s3_datasheet_en.pdf), Table 5-4, gives digital input high up to VDD+0.3 V: at assumed 3.3 V I/O supply that is 3.6 V. The separate 3.6 V absolute-maximum entry is for input power pins; do not cite it carelessly as an unconditional GPIO ceiling. [Arduino ADC documentation](https://docs.espressif.com/projects/arduino-esp32/en/latest/api/adc.html) gives S3 ADC_11db measurable range up to 3100 mV; current source selects it. Above-range input is a possible design concern, not established saturation/damage. [L298 datasheet](https://www.st.com/resource/en/datasheet/l298.pdf), Table 4, supports the conditional 1 A drop figures; no motor current/torque was measured by this audit.

## Predecessor record

V2 has camera-ready PDF/DOCX artifacts and a copyright/consent receipt naming the correct title and 2026 ICACT, dated 17 August 2026. RevA instructions record owner-reported V2 acceptance. This supports manuscript/workflow identity, not independently verified final acceptance/proceedings/DOI. An exact-title primary-domain lookup found no exact publication record; absence does not prove rejection. Do not confuse the rejected RevA manuscript with V2. Neutral author-manuscript citation can avoid inventing metadata while final details are checked.

Verified current V2 archive paths include `paper/Camera_Ready_Final_Submission_CRC_FINAL.docx`, `paper/Camera_Ready_Final_Submission_CRC_FINAL_PDFEXPRESS.pdf` and `paper/ICACT_788160_FINAL_PDFEXPRESS_PASSED.pdf`; hashes are in publication-source-hashes.json. These filenames indicate archived artifacts, not independent acceptance/publication evidence. The reference PDF inspected for title/content is also retained in RevA references.
