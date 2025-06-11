# Dynamic Journal Framework (SKSE Plugin)

---

## 1. Introduction

The Dynamic Journal Framework is an SKSE plugin that enables Skyrim books (vanilla or custom) to load their text from external `.txt` files.
This system provides modders and players with a flexible way to override or extend book content dynamically and modularly.

---

## 2. Features
* **Dynamic Book Content**
    * Load book text from external `.txt` files at runtime.
* **Persistent, Save-Aware Journals**
    Entries are saved to external .txt files, creating a permanent record of a character's journey.
    The framework is fully aware of the save/load system. Loading an older save will show the journal exactly as it was at that point in time.
    Your save history is recorded in the `_SaveHistory.log` file and the plugin builds the history chain needed.
* **Hybrid Content Model**
    * Mix static and dynamic content seamlessly
    * Text written outside of save blocks acts as a permanent template, always visible in the book.
    * Dynamic entries are saved in blocks and only appear if they are part of the current character's save history.
* **Simple Image & Text Formatting**
    * Intuitive Paragraphs: A single blank line in the .txt file creates a new paragraph.
    * Easy Image Embedding: Add images using a simple tag: [IMG=path|width|height].
    * Page Breaks: Create page breaks by leaving two or more blank lines.
* **API for Modders**
    Provides a C++/Papyrus API to dynamically add entries to any journal in response to game events.
* **INI-Based Configuration**
    Map in-game book titles to text files via a simple `DynamicBookFramework.ini` file.

---

## 3. Installation

1.  **Requirements:**
    * SKSE64 (Version matching your game version)
    * Address Library for SKSE Plugins
    * MCM Helper (Optional, but recommended for user configuration)
2.  Download and install the mod using Mod Organizer 2, Vortex, or manually by extracting to your Skyrim `Data` directory.
    ```
      Data/
      ├── DynamicBookFramework.esp          
      ├── SKSE/
      │   └── Plugins/
      │       ├── DynamicJournalFramework.dll
      │       ├── DynamicJournalFramework.ini
      │       └── books/  (Base directory for book texts)
      │             ├── PlayerChronicle.txt
      │             ├── my_custom_book.txt
      │             └── ... (other .txt files or subfolders)
      ├── MCM/
      │   └── Config/
      │       └── DynamicJournalFramework/  
      │           ├── config.json  
	  │           └── setting.ini
      └── Scripts/
          ├── DBF_MCMmenu.pex
          ├── DBF_ScriptUtil.pex             
          └── Source/                        
              ├── DBF_MCMmenu.psc
              └── DBF_ScriptUtil.psc
      ```
3.  Ensure the plugin is activated in your load order (e.g., `DynamicBookFramework.esp`, if used for the journal book item).

---

## 4. How to Use

### 4.1 For Players

* Any book listed in `Data/SKSE/Plugins/DynamicJournalFramework.ini` will load its content from the corresponding `.txt` file.
* Use the MCM menu to reload the book mapping without restarting the game if you add a new book while playing.

### 4.2 For Mod Authors / Content Creators

**Step 1: Create or Choose a Book**

* **Vanilla Book:** Just map the existing in-game title in the INI.
* **Custom Book:** Create a book in the Creation Kit and assign a unique **Title** (the name seen in-game). This title must exactly match the INI entry.
    * *Note:* Book text entered in the CK for a mapped book is ignored by this framework.

**Step 2: Create Your `.txt` Content File**

This file serves as the template and storage for your journal. You can mix permanent, static text with placeholders for dynamic entries.
* Place the file in your content directory.
  `Data/SKSE/Plugins/books/yourfile.txt`

* **Static vs. Dynamic Content:**
* Any text you write outside of a `;;BEGIN_SAVE_DATA;;` block is static. It will always appear in the book, no matter which save is loaded. Use this for titles, introductions, or decorations.
* Use the exposed Papyrus functions or the included API header file for you C++ project, to add entries automatically from your scripts.

* **Text Formatting Rules:**
    * `New line here.` (single newline in `.txt`) -> Becomes part of the paragraph above it. (using `\n` internally).
    * `(1 blank line in .txt)` -> Ends the current paragraph and starts a new one.
    * `(2+ blank lines in .txt)` -> Goes to a new page using `[pagebreak]`.
    * Images: Use the custom [IMG] tag on its own line.
        * Simple: [IMG=textures/my_mod/image.dds]
        * Specific: [IMG=textures/my_mod/map.png|290|389]
    Example .txt Template:
        ```
        The personal diary of Kaz'ra.
        May its pages remain safe.

        ------------------------------------

        ;;BEGIN_SAVE_DATA Save1_Kaz'ra_...;;
        My first entry. I arrived in Skyrim today. It is cold.
        ;;END_SAVE_DATA;;

        ;;BEGIN_SAVE_DATA Save2_Kaz'ra_...;;
        I fought a dragon at the Western Watchtower. I barely survived.
        [IMG=textures/creatures/dragon.dds|400|300]
        ;;END_SAVE_DATA;;

        ;;BEGIN_SAVE_DATA Save3_Kaz'ra_...;;
        I found a strange amulet in a cave today. It hums with a faint power.
        ;;END_SAVE_DATA;;
        ```
    In this example, the title and the decorative line are static and will always be visible. The entries themselves will appear or disappear correctly as you load Save1, Save2, or Save3.


**Step 3: Map Your Book in the INI**

* Example `DynamicJournalFramework.ini`:
    ```ini
    [Books]
    ; Example 1: A simple book directly in the 'books' folder
    My First Custom Book = my_first_custom_book.txt
    ; -> This will look for Data/SKSE/Plugins/books/my_first_custom_book.txt
    
    ; Example 2: A vanilla book you want to override
    Report: Disaster at Ionith = vanilla_overrides/disaster_at_ionith_override.txt
    ; -> This will look for Data/SKSE/Plugins/books/vanilla_overrides/disaster_at_ionith_override.txt
    ```

**For Papyrus Scripters**
The framework also exposes a Papyrus native function to allow your other scripts to append content to any dynamic book's `.txt` file. This is particularly useful for quest logs, diaries that update with quest progression, or any scenario where you want to add text to a book programmatically.
* **Function:** `DBF_ScriptUtil.AppendToFile(string asBookTitleKey, string asTextToAppend)`
    * `asBookTitleKey`: The title of the book as defined in your `DynamicJournalFramework.ini`. The function will resolve this title to the correct `.txt` file path using your INI mappings.
    * `asTextToAppend`: The plain text string you want to append to the book's content file. This text will be added to the end of the existing content in the `.txt` file (with a preceding newline if the file is not empty). The next time the book is opened, this new text will be processed by the framework's HTML markup rules.

* **Example Papyrus Usage:**
    * First add `<Import>C:\SSE\Mods\Dynamic Book Framework\Scripts\Source</Import>` in your Skyrim.ppj file's import section
    ```Skyrimse.ppj example
    <Imports>
        <Import>C:\Steam\steamapps\common\Skyrim Special Edition\Data\Scripts\Source</Import>
        <Import>C:\SSE\Mods\PapyrusUtil SE - Modders Scripting Utility Functions\Scripts\Source</Import>
        <Import>C:\SSE\Mods\Skyrim Script Extender (SKSE64)\Scripts\Source</Import>
        <Import>C:\SSE\Mods\MCM SDK\Source\Scripts</Import>
        <Import>C:\SSE\Mods\Dynamic Book Framework\Scripts\Source</Import>
    </Imports>
    ```
    * Add the function to your script.
    ```papyrus
    DBF_ScriptUtil.AppendToFile("My Adventures", "Today, I bravely faced a draugr and lived to tell the tale.")
    ```
    This would find the `.txt` file associated with the book titled "My Adventures" in your INI and append the new sentence to it.

**For C++ Devolloper**
Dynamic Book Framework exposes its functionality through the standard SKSE Messaging Interface. This allows your C++ plugin to safely send messages to add journal entries.
    .
* **The API defines two key components:**
        * APIMessageType: An enum that specifies the action you want to perform. Currently, the only action is kAppendEntry.
        * AppendEntryMessage: A C-style struct that holds the data required for the action, such as the book title and the text to add.
* **Example from my Dynamic Journal mod:**
    * To get started, simply include the provided API.h file in your project
    ```C++
    #include "API.h" // The API header for DynamicBookFramework
    ```
    * Construct and Dispatch the Message
    When you want to add an entry, create an AppendEntryMessage struct, fill it with your data, and use the messaging interface to Dispatch it.

    ```
    void AddEntryToJournal(const std::string& entryText) {
        auto* messaging = SKSE::GetMessagingInterface();
        if (!messaging) {
            // SKSE not available
            return;
        }

        // 1. Prepare the message data
        std::string timestamp = GetSkyrimDate();
        
        std::string playerLocationName = "parts unknown";
        if (auto* player = RE::PlayerCharacter::GetSingleton(); player && player->GetCurrentLocation()) {
            playerLocationName = player->GetCurrentLocation()->GetFullName() ? player->GetCurrentLocation()->GetFullName() : "an unknown area";
        }

        // Format the final entry string.
        std::string finalEntryText = std::format("{} - {}:\n{}", timestamp, playerLocationName, entryText);
        DynamicBookFramework_API::AppendEntryMessage message;
        message.bookTitleKey = "My Journal"; // The title of the journal book from the INI
        message.textToAppend = finalEntryText.c_str();

        // 2. Dispatch the message to your framework
        messaging->Dispatch(
            DynamicBookFramework_API::kAppendEntry,      // The message type
            &message,                                    // A pointer to our data struct
            sizeof(message),                             // The size of our data struct
            DynamicBookFramework_API::FrameworkPluginName  // The name of the DLL to receive the message
        );
    }
    ```
    
---

## 5. Compatibility

* Works with most mods, as it only overrides the content of mapped books.
* Compatible with vanilla and modded books.
* Requires Address Library and SKSE for your game version.

---

## 6.Recommended mods


---

## 7. Known Issues

* Complex HTML (e.g., deeply nested tags) may not render perfectly due to Scaleform limitations.
* Image display may still require careful path and format verification to work reliably, still unstable.

---

## 8. Planned Features

* An RDR2 style Personal Journal that logs events dynamically (e.g., sleeping, quests, locations).
* Investigating feasibility of player-written journal entries.
* Allow customization of the journal "voice" via MCM.
* Conditional templating for journal entry generation.
* Custom book.swf file for real-time book content update.

---

## 9. Credits

* SKSE Team — for the Script Extender.
* CharmedBayron, Ryan McKenzie and powerof3 for CommonlibSSE-NG.
* Mrowr Purr for her amazing guide videos.
* shazdeh2 for his amazing mod that was the inspiration of this work - Dynamic Books - A modder resource
* MCM Helper authors.
* the good people of the RE discord.
* Everyone who contributed code, feedback, or support.
