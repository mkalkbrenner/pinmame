#pragma once

#include "DisplayInfo.h"
#include <vector>

using namespace std;

class CDisplayInfoList {
  private:
    /************************************************
	 * Member Variables
	 ***********************************************/
    vector<CDisplayInfo> mDisplays;

  public:
    /************************************************
	 * Constructors
	 ***********************************************/
    CDisplayInfoList(void);

    /************************************************
	 * Destructor
	 ***********************************************/
    ~CDisplayInfoList(void);

    /************************************************
	 * Methods
	 ***********************************************/
    void AddDisplay(GUID FAR* lpGuid, LPSTR lpDriverDesc, LPSTR lpDriverName);
    BOOL Enumerate();

    /************************************************
	 * Properties
	 ***********************************************/
    CDisplayInfo* Item(size_t index);
    size_t Count(void) const;
};
