#include "AI/ScriptDevAI/include/sc_common.h"
enum { SAY_AGGRO = -1033037, SAY_SLAY = -1033038, SAY_DEATH = -1033039, SPELL_CLEAVE = 11609, SPELL_HAMSTRING = 1715, NPC_DEATHSWORN_CAPTAIN = 3872 };
struct boss_deathsworn_captainAI : public ScriptedAI
{
    boss_deathsworn_captainAI(Creature* pCreature) : ScriptedAI(pCreature) { Reset(); }
    uint32 m_uiCleaveTimer, m_uiHamstringTimer;
    void Reset() override { m_uiCleaveTimer = urand(6000, 9000); m_uiHamstringTimer = urand(9000, 14000); }
    void Aggro(Unit* pWho) override { DoScriptText(SAY_AGGRO, m_creature); }
    void KilledUnit(Unit* pVictim) override { DoScriptText(SAY_SLAY, m_creature); }
    void JustDied(Unit* pKiller) override { DoScriptText(SAY_DEATH, m_creature); }
    void UpdateAI(const uint32 uiDiff) override
    {
        if (!m_creature->SelectHostileTarget() || !m_creature->GetVictim()) return;
        if (m_uiCleaveTimer < uiDiff) { if (DoCastSpellIfCan(m_creature->GetVictim(), SPELL_CLEAVE) == CAST_OK) m_uiCleaveTimer = urand(7000, 11000); } else m_uiCleaveTimer -= uiDiff;
        if (m_uiHamstringTimer < uiDiff) { if (DoCastSpellIfCan(m_creature->GetVictim(), SPELL_HAMSTRING) == CAST_OK) m_uiHamstringTimer = urand(10000, 15000); } else m_uiHamstringTimer -= uiDiff;
        DoMeleeAttackIfReady();
    }
};
UnitAI* GetAI_boss_deathsworn_captain(Creature* pCreature) { return new boss_deathsworn_captainAI(pCreature); }
void AddSC_boss_deathsworn_captain() { Script* pNewScript = new Script; pNewScript->Name = "boss_deathsworn_captain"; pNewScript->GetAI = &GetAI_boss_deathsworn_captain; pNewScript->RegisterSelf(); }

