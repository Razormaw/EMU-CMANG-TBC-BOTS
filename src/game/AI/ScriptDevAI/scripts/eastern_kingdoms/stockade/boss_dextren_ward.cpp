/* CMaNGOS TBC - The Stockade (map 34): Dextren Ward */

#include "AI/ScriptDevAI/include/sc_common.h"

enum
{
    NPC_DEFIAS_INMATE             = 1727,   // si no existe en tu DB, cambia el entry o elimina el bloque de invocación
    SPELL_REND                    = 11572,
    SPELL_NET                     = 6534,

    SAY_AGGRO                     = -1034010,
    SAY_SUMMON                    = -1034011,
    SAY_SLAY                      = -1034012,
    SAY_DEATH                     = -1034013,
};

struct boss_dextren_wardAI : public ScriptedAI
{
    boss_dextren_wardAI(Creature* pCreature) : ScriptedAI(pCreature) { Reset(); }

    uint32 m_uiRendTimer;
    uint32 m_uiNetTimer;
    bool m_bSummoned;

    void Reset() override
    {
        m_uiRendTimer = urand(8000, 12000);
        m_uiNetTimer = urand(14000, 19000);
        m_bSummoned = false;
    }

    void Aggro(Unit* pWho) override { DoScriptText(SAY_AGGRO, m_creature); }
    void KilledUnit(Unit* pVictim) override { DoScriptText(SAY_SLAY, m_creature); }
    void JustDied(Unit* pKiller) override { DoScriptText(SAY_DEATH, m_creature); }

    void JustSummoned(Creature* pSummoned) override
    {
        if (Unit* pTarget = m_creature->GetVictim())
            pSummoned->AI()->AttackStart(pTarget);
    }

        void UpdateAI(const uint32 uiDiff) override
    {
        if (!m_creature->SelectHostileTarget() || !m_creature->GetVictim())
            return;

                if (!m_bSummoned && m_creature->GetHealthPercent() < 50.0f)
        {
            DoScriptText(SAY_SUMMON, m_creature);

            float fX, fY, fZ;
            for (uint8 i = 0; i < 2; ++i)
            {
                m_creature->GetNearPoint(m_creature, fX, fY, fZ, 0.0f, 5.0f, frand(0.0f, M_PI_F * 2.0f));
                m_creature->SummonCreature(NPC_DEFIAS_INMATE, fX, fY, fZ, m_creature->GetOrientation(), TEMPSPAWN_TIMED_OOC_DESPAWN, 60000);
            }
            m_bSummoned = true;
        }

        if (m_uiRendTimer < uiDiff)
        {
            if (DoCastSpellIfCan(m_creature->GetVictim(), SPELL_REND) == CAST_OK)
                m_uiRendTimer = urand(8000, 12000);
        }
        else
            m_uiRendTimer -= uiDiff;

        if (m_uiNetTimer < uiDiff)
        {
            if (DoCastSpellIfCan(m_creature->GetVictim(), SPELL_NET) == CAST_OK)
                m_uiNetTimer = urand(14000, 19000);
        }
        else
            m_uiNetTimer -= uiDiff;

        DoMeleeAttackIfReady();
    }
};

UnitAI* GetAI_boss_dextren_ward(Creature* pCreature)
{
    return new boss_dextren_wardAI(pCreature);
}

void AddSC_boss_dextren_ward()
{
    Script* pNewScript = new Script;
    pNewScript->Name = "boss_dextren_ward";
    pNewScript->GetAI = &GetAI_boss_dextren_ward;
    pNewScript->RegisterSelf();
}
