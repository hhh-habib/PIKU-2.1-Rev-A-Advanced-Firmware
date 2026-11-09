# PIKU reviewer audit reconciliation

## Current authority: Task 3 checkpoint, 9 November 2026

Read the [Task 3 methodology and physical-angle record](07_TASK_03_METHODOLOGY_AND_TURN_ANGLES_20261109.md) for the owner-confirmed pilot protocol, separate corridor tests, 40 turn-angle measurements and firmware identity reconciliation. The [Task 1–2 record](06_TASKS_01_02_EVIDENCE_PRESERVATION_20261109.md) retains original counts, voltage evidence, revisions and media limitations. Its checksum manifest is unchanged.

**Firmware project: PIKU 2.1 Rev A Software Development (OWNER-CONFIRMED). Exact tested binary/commit: NOT INDEPENDENTLY VERIFIED.** Reviewed source is master `004cf73521de2e67470bad40648e09770befda53`, PlatformIO `esp32-s3-n16r8`, before this documentation/data-only checkpoint. Existing local images have been hashed; no experiment-linked upload/readback was located. Archived build/SHA placeholders do not make project identity unknown.

The owner confirms indoor AUTO tests at 80%, a stopwatch-controlled 60 s observation limit, approximately 40 cm starts for T01–T10, inside-area starts for T11–T20, and success as autonomous travel more than 30 cm beyond the obstacle after passing it. Primary results remain 20/20 goals, 23 contacts, 6 recovery-activation entries and zero reported human interventions. T21–T22 are separate corridor escapes (2/2, 12 contacts, no reported intervention). The 40 angle readings form four separate characterization groups and are not measurements during those corridor tests.

The B0–B4 500-encounter aggregates retain their historical scope. Statements below about unmeasured recovery time concern that historical comparison, not the new pilot's owner-confirmed observation limit. The following earlier review remains dated history; its listed HEAD is not the current checkout. No paper, firmware, hardware or original evidence is edited by this checkpoint.

## Earlier reviewer reconciliation

Read-only review: 9 November 2026, Asia/Dhaka. Mission: `mission-20261109-051052-127894b6`.

ব্রো, সবচেয়ে জরুরি সংশোধন Fig. 6-এর statistics। তুমি নিশ্চিত করেছ—প্রতিটি ধাক্কা আলাদা গোনা হয়েছে। তাই ৩৮টি ধাক্কা মানে ৩৮% encounter-এ collision নয়; binomial Wilson interval-ও প্রযোজ্য নয়। আগের সম্পাদনায় graph রেখে দেওয়া ভুল ছিল। সংখ্যা অক্ষত রেখে মোট contact-event bar chart প্রস্তাব করেছি। এই ধাপে পেপার বদলাইনি।

Angle ও gap ruler/protractor দিয়ে মাপা। Recovery সফল মানে বাধা থেকে বের হয়ে চলতে পারা; সময় মাপা হয়নি। পুরোনো 0.55 m / 20 s / 40 s নিয়ম physical protocol হিসেবে ফেরানো হবে না। Confirmation-এর তারিখ 9 November; এটা পরীক্ষার তারিখ নয়।

| বিষয় | যাচাইয়ের ফল | পরের কাজ |
|---|---|---|
| B0–B4 ফল | Owner-confirmed physical; 25 aggregate claims, মোট 500 reported encounters; সংখ্যা সংরক্ষিত | Actual method ও পুরোনো firmware পরিচয়ের ফাঁক লিখতে হবে; কল্পিত trial log নয় |
| Wilson graph | Contact-event totals-এর জন্য অনুপযুক্ত | Error bars ও collision-probability wording বাদ; raw event-count chart |
| Nov 7 বনাম Oct 18 | আলাদা revision-এর software record | Dated ফল আলাদা থাকবে; count আলাদা হওয়া নিজে থেকে বিরোধ নয় |
| এখানে পাওয়া ZIP | 49 nonprivate production file current tree-এর সঙ্গে byte-identical; Nov 7 docs-ও আছে | Audit-এর ভিন্ন নামে উল্লেখ করা Nov 2 ZIP-এর সঙ্গে এক বলে ধরে নেওয়া হয়নি |
| Measurement | Ruler/protractor; recovery-তে সময় মাপা হয়নি | Known method লিখতে হবে; resolution/setup বানানো যাবে না |
| V2 predecessor | Camera-ready artifacts ও ICACT copyright record আছে; নথিতে owner-reported acceptance | RevA rejection-এর সঙ্গে মেশানো যাবে না; final publication metadata এখনও verified নয় |
| Cost | Current paper ইতিমধ্যে rounded estimate বলছে; historical estimate 5,005 | প্রায় 5,000 থাকবে; rounding note যথেষ্ট |
| Dashboard | Full originals আছে; owner cropping চান না | তিনটি full capture পড়ার মতো করে ছড়িয়ে দেওয়ার proposed layout |
| Contribution | Complementary sensing, fresh readings, recovery ও hazard/operator coordination | Engineering integration পরিষ্কার করা; নতুন AI/SLAM দাবি নয় |

## Five requested outputs

1. [Controller provenance](01_CONTROLLER_PROVENANCE.md)
2. [500-encounter claim register](02_500_TRIAL_CLAIM_REGISTER.csv) — [units and explanation](02_REGISTER_README.md)
3. [Source-to-results matrix](03_SOURCE_TO_RESULTS_MATRIX.csv) — [source identity and references](03_SOURCE_IDENTITY.md)
4. [Section correction plan](04_SECTION_CORRECTION_PLAN.md)
5. [Six-page layout budget](05_SIX_PAGE_LAYOUT_BUDGET.md)

Every matrix claim is independently supported, author-reported, calculated or unresolved, with limitations. Independently supported may mean checked implementation/record; it does not imply a new physical replication. Old Wilson arithmetic is explicitly marked inapplicable, not valid publication evidence.

## Scope and preservation

Target: `paper/PIKU_RevA_Team_Based_Revision_20261109.docx`, SHA256 `f5fa182d9a59fc4604e1bdd1083364f5becfc1618f7d608d21d30766b66ce412`.

Existing papers, figures, firmware, evidence and Git history were not edited. No tests, builds, flashes, commits or pushes were run. Current source/results were snapshotted through nonprivate manifests and extracts in this separate audit folder; no shareable full private firmware build was created.

Search scope: RevA source/docs/tests, physical Docs/manuscript history/figures, local generic software ZIP, prior Nov 7 preservation/mission records, targeted V2 publication authorities and a bounded exact-title primary-domain lookup. The exact audit-named Nov 2 ZIP was not located in the project/attachment/Downloads-root search. No raw 500-trial series or comparator binary mapping was located in inspected evidence; this does not establish that such records do not exist elsewhere.

Next step: owner review of the correction/layout plan. Manuscript rewriting and figure replacement await that review. Remaining methods/build/publication questions do not automatically require repeating all 500 encounters.
