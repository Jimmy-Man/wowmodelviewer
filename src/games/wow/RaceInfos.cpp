#include "RaceInfos.h"

#include "Game.h"
#include "WoWDatabase.h"
#include "WoWModel.h"

#include "logger/Logger.h"

#define DEBUG_RACEINFOS 0

std::map<int, RaceInfos> RaceInfos::RACES;

bool RaceInfos::getCurrent(WoWModel * model, RaceInfos & result)
{
  if (!model)
  {
    LOG_ERROR << __FUNCTION__ << "model is null";
    return false;
  }

  auto raceInfosIt = RACES.find(model->gamefile->fileDataId());
  if(raceInfosIt != RACES.end())
  {
    result = raceInfosIt->second;
    return true;
  }

  LOG_ERROR << "Unable to retrieve race infos for model" << model->gamefile->fullname() << model->gamefile->fileDataId();
 
  return false;
}

void RaceInfos::init()
{
  sqlResult races = 
    GAMEDATABASE.sqlQuery("SELECT CMDM.FileID as malemodel, lower(ClientPrefix), CharComponentTexLayoutID, "
                          "CMDF.FileID AS femalemodel, lower(ClientPrefix), CharComponentTexLayoutID, "
                          "CMDMHD.FileID as malemodelHD, lower(ClientPrefix), CharComponentTexLayoutHiResID, "
                          "CMDFHD.FileID AS femalemodelHD, lower(ClientPrefix), CharComponentTexLayoutHiResID, "
                          "ChrRaces.ID, ItemDisplayRaceID FROM ChrRaces "
                          "LEFT JOIN CreatureDisplayInfo CDIM ON CDIM.ID = MaleDisplayID LEFT JOIN CreatureModelData CMDM ON CDIM.ModelID = CMDM.ID "
                          "LEFT JOIN CreatureDisplayInfo CDIF ON CDIF.ID = FemaleDisplayID LEFT JOIN CreatureModelData CMDF ON CDIF.ModelID = CMDF.ID "
                          "LEFT JOIN CreatureDisplayInfo CDIMHD ON CDIMHD.ID = HighResMaleDisplayId LEFT JOIN CreatureModelData CMDMHD ON CDIMHD.ModelID = CMDMHD.ID "
                          "LEFT JOIN CreatureDisplayInfo CDIFHD ON CDIFHD.ID = HighResFemaleDisplayId LEFT JOIN CreatureModelData CMDFHD ON CDIFHD.ModelID = CMDFHD.ID");

  if(!races.valid || races.empty())
  {
    LOG_ERROR << "Unable to collect race information from game database";
    return;
  }

  for(int i=0, imax = races.values.size() ; i < imax ; i++)
  {
    std::string displayPrefix = "";
    if (races.values[i][13].toInt() != 0)
    {
      QString query = QString("SELECT lower(ClientPrefix) FROM ChrRaces WHERE ID = %1").arg(races.values[i][13].toInt());
      sqlResult display = GAMEDATABASE.sqlQuery(query);
      displayPrefix = display.values[0][0].toStdString();
    }

    for(int r = 0; r <12 ; r+=3)
    {
      if(races.values[i][r] != "")
      {
        RaceInfos infos;
        infos.prefix = (displayPrefix != "") ? displayPrefix : races.values[i][r + 1].toStdString();
        infos.textureLayoutID = races.values[i][r+2].toInt();
        infos.raceid = races.values[i][12].toInt();
        infos.sexid = (r == 0 || r == 6)?0:1;
      
        int modelfileid = races.values[i][r].toInt();
        
        if ((r == 6) || (r == 9)) // if we are dealing with a HD model
          infos.isHD = true;
        else
          infos.isHD = false;

        if (RACES.find(modelfileid) == RACES.end())
          RACES[modelfileid] = infos;

#if DEBUG_RACEINFOS > 0
        LOG_INFO << "---------------------------";
        LOG_INFO << "modelfileid ->" << modelfileid;
        LOG_INFO << "infos.prefix =" << infos.prefix.c_str();
        LOG_INFO << "infos.textureLayoutID =" << infos.textureLayoutID;
        LOG_INFO << "infos.raceid =" << infos.raceid;
        LOG_INFO << "infos.sexid =" << infos.sexid;
        LOG_INFO << "infos.customization[0] =" << infos.customization[0].c_str();
        LOG_INFO << "infos.customization[1] =" << infos.customization[1].c_str();
        LOG_INFO << "infos.customization[2] =" << infos.customization[2].c_str();
        LOG_INFO << "infos.isHD =" << infos.isHD;
        LOG_INFO << "---------------------------";
#endif
      }
    }
  }

}

int RaceInfos::getHDModelForFileID(int fileid)
{
  int result = fileid; // return same file id by default

  auto it = RACES.find(fileid);
  if ((it != RACES.end()) && (it->second.isHD == false))
  {
    int raceid = it->second.raceid;
    int sexid = it->second.sexid;

    for (auto it = RACES.begin(), itEnd = RACES.end(); it != itEnd; ++it)
    {
      if ((it->second.raceid == raceid) && (it->second.sexid == sexid) && (it->second.isHD == true))
      {
        result = it->first;
        break;
      }
    }
  }

  return result;
}
