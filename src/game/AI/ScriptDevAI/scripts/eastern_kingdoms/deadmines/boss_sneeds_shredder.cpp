#include "AI/ScriptDevAI/include/sc_common.h"
enum { EMOTE_AGGRO = -1036007, EMOTE_DEATH = -1036008, SPELL_CLEAVE = 11609, SPELL_ENRAGE = 8599, NPC_SNEEDS_SHREDDER = 642 };
struct boss_sneeds_shredderAI : public ScriptedAI
{
    boss_sneeds_shredderAI(Creature* pCreature) : ScriptedAI(pCreature) { Reset(); }
    uint32 m_uiCleaveTimer; bool m_bEnraged;
    void Reset() override { m_uiCleaveTimer = urand(6000, 9000); m_bEnraged = false; }
    void Aggro(Unit* pWho) override { DoScriptText(EMOTE_AGGRO, m_creature); }
    void JustDied(Unit* pKiller) override { DoScriptText(EMOTE_DEATH, m_creature); }
    void UpdateAI(const uint32 uiDiff) override
    {
        if (!m_creature->SelectHostileTarget() || !m_creature->GetVictim()) return;
        if (!m_bEnraged && m_creature->GetHealthPercent() < 30.0f) { if (DoCastSpellIfCan(m_creature, SPELL_ENRAGE) == CAST_OK) m_bEnraged = true; }
        if (m_uiCleaveTimer < uiDiff) { if (DoCastSpellIfCan(m_creature->GetVictim(), SPELL_CLEAVE) == CAST_OK) m_uiCleaveTimer = urand(7000, 11000); } else m_uiCleaveTimer -= uiDiff;
        DoMeleeAttackIfReady();
    }
};
UnitAI* GetAI_boss_sneeds_shredder(Creature* pCreature) { return new boss_sneeds_shredderAI(pCreature); }
void AddSC_boss_sneeds_shredder() { Script* pNewScript = new Script; pNewScript->Name = "boss_sneeds_shredder"; pNewScript->GetAI = &GetAI_boss_sneeds_shredder; pNewScript->RegisterSelf(); }

