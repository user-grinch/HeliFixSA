#include <CPools.h>
#include <assembly.hpp>
#include <extensions/ScriptCommands.h>
#include <extensions/scripting/ScriptCommandNames.h>
#include <CHeli.h>
#include <CWorld.h>

#define NEWS_CHOPPER 218
#define COP_CHOPPER 165
#define POL_MAV 227
#define COP_CHAR 2
#define NEWS_CHAR 33
#define PILOT_CHAR 33

using namespace plugin;
CdeclEvent<AddressList<0x4A451A, H_CALL>, PRIORITY_BEFORE, ArgPickNone, void()> updateHeliEvent;

class HeliFix
{
public:

	void SetupHeliCrash(CHeli *&pHeli)
	{
		if (pHeli && !(pHeli->m_pDriver && pHeli->m_pDriver->m_fHealth > 1.0))
		{
			pHeli->m_autoPilot.m_nCarMission = MISSION_HELI_LAND;
			pHeli->m_fHealth = 200.0f; // decrease health to ensure explosion on land

			CPed *pDriver = pHeli->m_pDriver;
			if(pDriver)
			{
				Command<Commands::MARK_CHAR_AS_NO_LONGER_NEEDED>(CPools::GetPedRef(pDriver));
			}

			CPed *pPass = pHeli->m_passengers[0];
			if (pPass)
			{
				Command<Commands::MARK_CHAR_AS_NO_LONGER_NEEDED>(CPools::GetPedRef(pPass));
			}
			pHeli = nullptr;
		}
	}

	HeliFix()
	{
		// add driver and a passenger in helis
		injector::MakeInline<0x5ACD8D, 0x5ACD92>([](injector::reg_pack& regs)
		{
			CVehicle *pVeh = (CVehicle*)regs.esi;
			int hVeh = CPools::GetVehicleRef(pVeh);
			CWorld::Add(pVeh);

			if (pVeh->m_nModelIndex == COP_CHOPPER || pVeh->m_nModelIndex == POL_MAV)
			{
				int hDriver, hPass;
				Command<Commands::REQUEST_MODEL>(COP_CHAR);
				Command<Commands::LOAD_ALL_MODELS_NOW>();
				Command<Commands::CREATE_CHAR_INSIDE_CAR>(hVeh, PEDTYPE_COP, COP_CHAR, &hDriver);
				pVeh->m_pDriver = CPools::GetPed(hDriver);
				Command<Commands::CREATE_CHAR_AS_PASSENGER>(hVeh, PEDTYPE_COP, COP_CHAR, 0, &hPass);
				pVeh->m_passengers[0] = CPools::GetPed(hPass);
				Command<Commands::MARK_MODEL_AS_NO_LONGER_NEEDED>(COP_CHAR);
			}
			else
			{
				int hDriver, hPass;
				Command<Commands::REQUEST_MODEL>(PILOT_CHAR);
				Command<Commands::REQUEST_MODEL>(NEWS_CHAR);
				Command<Commands::LOAD_ALL_MODELS_NOW>();
				Command<Commands::CREATE_CHAR_INSIDE_CAR>(hVeh, PEDTYPE_CIVMALE, PILOT_CHAR, &hDriver);
				pVeh->m_pDriver = CPools::GetPed(hDriver);
				if (pVeh->m_nModelIndex == NEWS_CHOPPER)
				{
					Command<Commands::CREATE_CHAR_AS_PASSENGER>(hVeh, PEDTYPE_CIVMALE, NEWS_CHAR, 0, &hPass);
					pVeh->m_passengers[0] = CPools::GetPed(hPass);
				}

				Command<Commands::MARK_MODEL_AS_NO_LONGER_NEEDED>(NEWS_CHAR);
				Command<Commands::MARK_MODEL_AS_NO_LONGER_NEEDED>(PILOT_CHAR);
			}
		});

		// Lose heli control on driver death
		updateHeliEvent += [this]()
		{
			SetupHeliCrash(CHeli::pHelis[0]);
			SetupHeliCrash(CHeli::pHelis[1]);
		};

	};
} heliFix;
