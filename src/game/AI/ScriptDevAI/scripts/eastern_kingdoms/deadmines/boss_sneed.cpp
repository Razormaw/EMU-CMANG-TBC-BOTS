#include "AI/ScriptDevAI/include/sc_common.h"
enum { EMOTE_AGGRO = -1036009, EMOTE_DEATH = -1036010, SPELL_CLEAVE = 11609, SPELL_HAMSTRING = 1715, NPC_SNEED = 643 };
struct boss_sneedAI : public ScriptedAI
{
    boss_sneedAI(Creature* pCreature) : ScriptedAI(pCreature) { Reset(); }
    uint32 m_uiCleaveTimer, m_uiHamstringTimer;
    void Reset() override { m_uiCleaveTimer = urand(6000, 9000); m_uiHamstringTimer = urand(9000, 14000); }
    void Aggro(Unit* pWho) override { DoScriptText(EMOTE_AGGRO, m_creature); }
    void JustDied(Unit* pKiller) override { DoScriptText(EMOTE_DEATH, m_creature); }
    void UpdateAI(const uint32 uiDiff) override
    {
        if (!m_creature->SelectHostileTarget() || !m_creature->GetVictim()) return;
        if (m_uiCleaveTimer < uiDiff) { if (DoCastSpellIfCan(m_creature->GetVictim(), SPELL_CLEAVE) == CAST_OK) m_uiCleaveTimer = urand(7000, 11000); } else m_uiCleaveTimer -= uiDiff;
        if (m_uiHamstringTimer < uiDiff) { if (DoCastSpellIfCan(m_creature->GetVictim(), SPELL_HAMSTRING) == CAST_OK) m_uiHamstringTimer = urand(10000, 15000); } else m_uiHamstringTimer -= uiDiff;
        DoMeleeAttackIfReady();
    }
};
UnitAI* GetAI_boss_sneed(Creature* pCreature) { return new boss_sneedAI(pCreature); }
void AddSC_boss_sneed() { Script* pNewScript = new Script; pNewScript->Name = "boss_sneed"; pNewScript->GetAI = &GetAI_boss_sneed; pNewScript->RegisterSelf(); }

