#include "AI/ScriptDevAI/include/sc_common.h"

enum { SAY_AGGRO = -1070011, SAY_SLAY = -1070012, SAY_DEATH = -1070013, SPELL_EARTH_SHOCK = 8045, SPELL_HEALING_WAVE = 12491, NPC_GRIMLOK = 4854 };

struct boss_grimlokAI : public ScriptedAI
{
    boss_grimlokAI(Creature* pCreature) : ScriptedAI(pCreature) { Reset(); }
    uint32 m_uiShockTimer, m_uiHealTimer;
    void Reset() override { m_uiShockTimer = urand(6000, 10000); m_uiHealTimer = urand(12000, 18000); }
    void Aggro(Unit* pWho) override { DoScriptText(SAY_AGGRO, m_creature); }
    void KilledUnit(Unit* pVictim) override { DoScriptText(SAY_SLAY, m_creature); }
    void JustDied(Unit* pKiller) override { DoScriptText(SAY_DEATH, m_creature); }
    void UpdateAI(const uint32 uiDiff) override
    {
        if (!m_creature->SelectHostileTarget() || !m_creature->GetVictim()) return;
        if (m_uiShockTimer < uiDiff)
        { if (DoCastSpellIfCan(m_creature->GetVictim(), SPELL_EARTH_SHOCK) == CAST_OK) m_uiShockTimer = urand(8000, 12000); }
        else m_uiShockTimer -= uiDiff;
        if (m_creature->GetHealthPercent() < 60.0f && m_uiHealTimer < uiDiff)
        { if (DoCastSpellIfCan(m_creature, SPELL_HEALING_WAVE) == CAST_OK) m_uiHealTimer = urand(15000, 22000); }
        else m_uiHealTimer -= uiDiff;
        DoMeleeAttackIfReady();
    }
};
UnitAI* GetAI_boss_grimlok(Creature* pCreature) { return new boss_grimlokAI(pCreature); }
void AddSC_boss_grimlok() { Script* pNewScript = new Script; pNewScript->Name = "boss_grimlok"; pNewScript->GetAI = &GetAI_boss_grimlok; pNewScript->RegisterSelf(); }

