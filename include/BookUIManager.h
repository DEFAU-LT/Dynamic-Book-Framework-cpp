//BookUIManager.h
#pragma once
#include "PCH.h" 

namespace DynamicBookFramework {

    namespace BookUIManager {
        
        // Attempts to refresh the content of the currently open dynamic book
        // by re-reading its associated .txt file and re-invoking the SetBookText logic.
        // Returns true if a refresh was attempted (i.e., book menu was open and it was a dynamic book).
        // Returns false if the book menu wasn't open, not a dynamic book, or an error occurred.
        bool RefreshCurrentlyOpenBook();

    } // namespace BookUIManager

}
