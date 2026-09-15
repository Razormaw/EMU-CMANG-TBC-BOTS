/* CMaNGOS TBC - Gnomeregan (map 90): Viscous Fallout */

#include "AI/ScriptDevAI/include/sc_common.h"
#include "gnomeregan.h"

enum
{
    EMOTE_AGGRO               = -1090032,
    EMOTE_DEATH               = -1090033,

    SPELL_TOXIC_VOLLEY        = 21687,    // VERIFICADO: volea de veneno con DoT
};

struct boss_viscous_falloutAI : public ScriptedAI
{
    boss_viscous_falloutAI(Creature* pCreature) : ScriptedAI(pCreature) { Reset(); }

    uint32 m_uiToxicVolleyTimer;

    void Reset() override
    {
        m_uiToxicVolleyTimer = urand(6000, 10000);
    }

    void Aggro(Unit* pWho) override { DoScriptText(EMOTE_AGGRO, m_creature); }
    void JustDied(Unit* pKiller) override { DoScriptText(EMOTE_DEATH, m_creature); }

    void UpdateAI(const uint32 uiDiff) override
    {
        if (!m_creature->SelectHostileTarget() || !m_creature->GetVictim())
            return;

        if (m_uiToxicVolleyTimer < uiDiff)
        {
            if (DoCastSpellIfCan(m_creature, SPELL_TOXIC_VOLLEY) == CAST_OK)
                m_uiToxicVolleyTimer = urand(9000, 14000);
        }
        else
            m_uiToxicVolleyTimer -= uiDiff;

        DoMeleeAttackIfReady();
    }
};

UnitAI* GetAI_boss_viscous_fallout(Creature* pCreature)
{
    return new boss_viscous_falloutAI(pCreature);
}

void AddSC_boss_viscous_fallout()
{
    Script* pNewScript = new Script;
    pNewScript->Name = "boss_viscous_fallout";
    pNewScript->GetAI = &GetAI_boss_viscous_fallout;
    pNewScript->RegisterSelf();
}

