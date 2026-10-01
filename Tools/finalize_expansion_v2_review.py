"""Record reproducible source/import/runtime evidence without claiming user approval."""
import json,re
from datetime import datetime,timezone
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
BASE=ROOT/'ArtSource/EnemyExpansion';HANDOFF=BASE/'HomePCHandoff-v2'
def read(p):return json.loads(p.read_text(encoding='utf-8-sig'))
def save(p,data):p.write_text(json.dumps(data,indent=2)+'\n')
inv=read(HANDOFF/'inventory.json');assert inv['remainingSheets']==0
verify=(ROOT/'Saved/Logs/ExpansionV2FinalVerify2.log').read_text(errors='replace')
match=re.search(r'EXPANSION_VERIFY checks=(\d+) failures=0',verify);assert match
import_log=(ROOT/'Saved/Logs/ExpansionV2Import.log').read_text(errors='replace')
assert 'EXPANSION_V2_IMPORT_COMPLETE 1105' in import_log
records=[]
for i,e in enumerate(inv['enemies']):
    folder=BASE/e['folder'];source=folder/'animation-v2';runtime=folder/'runtime-v2'
    audit=read(source/'crop-check.json')
    for check in audit['checks']:
        assert not check['empty'] and not check['overlappingMaskRows'],(e['folder'],check)
        assert not check['sourceBorderPoses'] and not check['cropWarnings'],(e['folder'],check)
        if check['components'] is not None:
            assert check['poses']==check['components'],(e['folder'],check)
    atlases=read(runtime/'runtime-manifest.json')
    for atlas in atlases:
        assert (ROOT/'Content/Art/EnemyExpansion'/e['folder']/(atlas['name']+'.uasset')).exists()
    spec=read(source/'enemy-spec.json')
    spec['assetStatus']='SPRITES_COMPLETE_UE_TEST_READY'
    spec['sourceProgress'].update(status='SPRITES_COMPLETE_UE_TEST_READY',ueImportTested=True)
    spec['runtimeReview']={
        'species':54+i,'atlasCount':len(atlases),'poseCount':sum(a['frames'] for a in atlases),
        'sourceCropAuditPassed':True,'atlasBoundsAuditPassed':True,'automatedUETestPassed':True,
        'visualReviewScope':'Directional source repair review, cross-state scale boards, eight-direction attack contact boards; automated combat captures for the five elites.',
        'userGameplayApproval':False,'exhaustiveManualGameplay':False,
        'timingAuthority':'Source/TheBeardAndBlade/DungeonExpansionV2.h::ReleaseFrame',
        'report':'ArtSource/EnemyExpansion/HomePCHandoff-v2/COMPLETION-AND-TEST.md'}
    save(source/'enemy-spec.json',spec)
    records.append({'enemy':e['folder'],**spec['runtimeReview']})
report={'createdUTC':datetime.now(timezone.utc).isoformat(),'status':'SPRITES_COMPLETE_UE_TEST_READY',
    'enemies':25,'selectedSourceSheets':1345,'missingSheets':0,'runtimeAtlases':sum(r['atlasCount'] for r in records),
    'runtimePoses':sum(r['poseCount'] for r in records),'automatedUEChecks':int(match.group(1)),'automatedUEFailures':0,
    'campaignSpawnTablesChanged':False,'githubPushed':False,'userGameplayApproval':False,'records':records}
assert report['runtimeAtlases']==1105 and report['runtimePoses']==14920
save(HANDOFF/'completion-report.json',report)
validation=read(HANDOFF/'validation.json');validation.update(ueTested=True,runtimeCompletionReport='completion-report.json');save(HANDOFF/'validation.json',validation)
print(f"Complete: {report['enemies']} enemies, {report['runtimePoses']} poses, {report['automatedUEChecks']} UE checks / 0 failures")
