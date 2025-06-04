# Dynamic Journal Framework (SKSE Plugin)

---

## 1. Introduction

The Dynamic Journal Framework is an SKSE plugin that enables Skyrim books (vanilla or custom) to load their text from external `.txt` files.
This system provides modders and players with a flexible way to override or extend book content dynamically and modularly.

---

## 2. Features

* **Dynamic Book Content**
    * Load book text from external `.txt` files at runtime.
* **INI-Based Mapping**
    * Map in-game book titles to text files via a simple `DynamicBookFramework.ini` file.
* **HTML Formatting Support**
    * Auto-formats plain text: newlines become visual line breaks within paragraphs, a single blank line creates a new paragraph, and two or more blank lines become a `[pagebreak]`.
    * Allows raw HTML blocks using `;;BEGIN_RAW_HTML;;` and `;;END_RAW_HTML;;` for advanced formatting or embedded images.
    * Lines starting with `<img src=...>` and the lines immediately before and after it are treated as raw HTML automatically.
* **MCM Integration (via MCM Helper)**
    * Provides a button to reload the INI file in-game, no restart required.

---

## 3. Installation

1.  **Requirements:**
    * SKSE64 (Version matching your game version)
    * Address Library for SKSE Plugins
    * MCM Helper (Optional, but recommended for user configuration)
2.  Download and install the mod using Mod Organizer 2, Vortex, or manually by extracting to your Skyrim `Data` directory.
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

* Place the file in your content directory.
  `Data/SKSE/Plugins/books/yourfile.txt`
* **Text Formatting Rules:**
    * `Plain line of text.` -> Becomes part of a paragraph.
    * `New line here.` (single newline in `.txt`) -> Becomes part of the paragraph above. (using `\n` internally).
    * `(1 blank line in .txt)` -> Ends the current paragraph and starts a new one.
    * `(2+ blank lines in .txt)` -> Goes to a new page using `[pagebreak]`.
    * Lines containing `<img src='img://...'>` (and the lines immediately before and after) are output raw.
* **Raw HTML Block Example:**
    ```html
    ;;BEGIN_RAW_HTML;;
    <p align='center'><b><font color='#FFAA00'>Special Layout</font></b></p>
    <img src='img://textures/myimage.dds' width='256' height='256'>
    ;;END_RAW_HTML;;
    ```

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
* Image paths in `<img>` tags must use the `img://` protocol, and supported formats (`.png`).
* Image display may still require careful path and format verification to work reliably, still unstable.

---

## 8. Planned Features

* An RDR2 style Personal Journal that logs events dynamically (e.g., sleeping, quests, locations).
* Investigating feasibility of player-written journal entries.
* Allow customization of the journal "voice" via MCM.
* Conditional templating for journal entry generation.

---

## 9. Credits

* SKSE Team — for the Script Extender.
* CharmedBayron, Ryan McKenzie and powerof3 for CommonlibSSE-NG.
* Mrowr Purr for her amazing guide videos.
* shazdeh2 for his amazing mod that was the inspiration of this work - Dynamic Books - A modder resource
* MCM Helper authors.
* the good people of the RE discord.
* Everyone who contributed code, feedback, or support.
