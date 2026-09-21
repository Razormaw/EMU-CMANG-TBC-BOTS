#include "AI/ScriptDevAI/include/sc_common.h"

enum { SAY_AGGRO = -1043023, SAY_SLAY = -1043024, SAY_DEATH = -1043025, SPELL_EARTH_SHOCK = 8045, SPELL_RENEW = 6078, NPC_LORD_PYTHAS = 3670 };

struct boss_lord_pythasAI : public ScriptedAI
{
    boss_lord_pythasAI(Creature* pCreature) : ScriptedAI(pCreature) { Reset(); }
    uint32 m_uiShockTimer, m_uiRenewTimer;
    void Reset() override { m_uiShockTimer = urand(6000, 10000); m_uiRenewTimer = urand(12000, 18000); }
    void Aggro(Unit* pWho) override { DoScriptText(SAY_AGGRO, m_creature); }
    void KilledUnit(Unit* pVictim) override { DoScriptText(SAY_SLAY, m_creature); }
    void JustDied(Unit* pKiller) override { DoScriptText(SAY_DEATH, m_creature); }
    void UpdateAI(const uint32 uiDiff) override
    {
        if (!m_creature->SelectHostileTarget() || !m_creature->GetVictim()) return;
        if (m_uiShockTimer < uiDiff)
        { if (DoCastSpellIfCan(m_creature->GetVictim(), SPELL_EARTH_SHOCK) == CAST_OK) m_uiShockTimer = urand(8000, 12000); }
        else m_uiShockTimer -= uiDiff;
        if (m_creature->GetHealthPercent() < 60.0f && m_uiRenewTimer < uiDiff)
        { if (DoCastSpellIfCan(m_creature, SPELL_RENEW) == CAST_OK) m_uiRenewTimer = urand(15000, 20000); }
        else m_uiRenewTimer -= uiDiff;
        DoMeleeAttackIfReady();
    }
};
UnitAI* GetAI_boss_lord_pythas(Creature* pCreature) { return new boss_lord_pythasAI(pCreature); }
void AddSC_boss_lord_pythas() { Script* pNewScript = new Script; pNewScript->Name = "boss_lord_pythas"; pNewScript->GetAI = &GetAI_boss_lord_pythas; pNewScript->RegisterSelf(); }

