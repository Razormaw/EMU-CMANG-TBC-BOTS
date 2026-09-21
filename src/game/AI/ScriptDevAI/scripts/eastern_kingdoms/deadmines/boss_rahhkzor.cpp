#include "AI/ScriptDevAI/include/sc_common.h"
enum { SAY_AGGRO = -1036004, SAY_SLAY = -1036005, SAY_DEATH = -1036006, SPELL_CLEAVE = 11609, SPELL_KNOCK_AWAY = 10101, NPC_RHAHKZOR = 644 };
struct boss_rahhkzorAI : public ScriptedAI
{
    boss_rahhkzorAI(Creature* pCreature) : ScriptedAI(pCreature) { Reset(); }
    uint32 m_uiCleaveTimer, m_uiKnockTimer;
    void Reset() override { m_uiCleaveTimer = urand(6000, 9000); m_uiKnockTimer = urand(12000, 16000); }
    void Aggro(Unit* pWho) override { DoScriptText(SAY_AGGRO, m_creature); }
    void KilledUnit(Unit* pVictim) override { DoScriptText(SAY_SLAY, m_creature); }
    void JustDied(Unit* pKiller) override { DoScriptText(SAY_DEATH, m_creature); }
    void UpdateAI(const uint32 uiDiff) override
    {
        if (!m_creature->SelectHostileTarget() || !m_creature->GetVictim()) return;
        if (m_uiCleaveTimer < uiDiff) { if (DoCastSpellIfCan(m_creature->GetVictim(), SPELL_CLEAVE) == CAST_OK) m_uiCleaveTimer = urand(7000, 11000); } else m_uiCleaveTimer -= uiDiff;
        if (m_uiKnockTimer < uiDiff) { if (DoCastSpellIfCan(m_creature->GetVictim(), SPELL_KNOCK_AWAY) == CAST_OK) m_uiKnockTimer = urand(14000, 18000); } else m_uiKnockTimer -= uiDiff;
        DoMeleeAttackIfReady();
    }
};
UnitAI* GetAI_boss_rahhkzor(Creature* pCreature) { return new boss_rahhkzorAI(pCreature); }
void AddSC_boss_rahhkzor() { Script* pNewScript = new Script; pNewScript->Name = "boss_rahhkzor"; pNewScript->GetAI = &GetAI_boss_rahhkzor; pNewScript->RegisterSelf(); }

