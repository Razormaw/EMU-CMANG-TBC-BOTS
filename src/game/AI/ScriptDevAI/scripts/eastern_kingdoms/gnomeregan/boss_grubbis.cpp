/* CMaNGOS TBC - Gnomeregan (map 90): Grubbis (invocado por el evento de Emi Shortfuse) */

#include "AI/ScriptDevAI/include/sc_common.h"
#include "gnomeregan.h"

enum
{
    SAY_AGGRO                 = -1090029,
    SAY_SLAY                  = -1090030,
    SAY_DEATH                 = -1090031,
};

struct boss_grubbisAI : public ScriptedAI
{
    boss_grubbisAI(Creature* pCreature) : ScriptedAI(pCreature) { Reset(); }

    void Reset() override {}

    void Aggro(Unit* pWho) override { DoScriptText(SAY_AGGRO, m_creature); }
    void KilledUnit(Unit* pVictim) override { DoScriptText(SAY_SLAY, m_creature); }
    void JustDied(Unit* pKiller) override { DoScriptText(SAY_DEATH, m_creature); }

    void UpdateAI(const uint32 uiDiff) override
    {
        if (!m_creature->SelectHostileTarget() || !m_creature->GetVictim())
            return;

        DoMeleeAttackIfReady();
    }
};

UnitAI* GetAI_boss_grubbis(Creature* pCreature)
{
    return new boss_grubbisAI(pCreature);
}

void AddSC_boss_grubbis()
{
    Script* pNewScript = new Script;
    pNewScript->Name = "boss_grubbis";
    pNewScript->GetAI = &GetAI_boss_grubbis;
    pNewScript->RegisterSelf();
}

