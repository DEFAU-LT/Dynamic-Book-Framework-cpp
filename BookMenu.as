import skse;
import skyui.util.GlobalFunctions;
import gfx.io.GameDelegate;
import Shared.GlobalFunc;
import skyui.util.Debug;
import flash.external.ExternalInterface;
import gfx.controls.TextInput;
import gfx.managers.FocusHandler;
import skyui.defines.Input;



class BookMenu extends MovieClip
{
	static var PAGE_BREAK_TAG: String = "[pagebreak]";
	
	static var NOTE_WIDTH: Number = 400;
	static var NOTE_X_OFFSET: Number = 20;
	static var NOTE_Y_OFFSET: Number = 10;
	static var CACHED_PAGES: Number = 4;
	
	static var BookMenuInstance: Object;

	var sCurrentBookTitle:String = "";
	var currentBookFormID:Number;

	//public var textInput: TextInput;
	
	/* Stage Elements */
	var ReferenceTextInstance: TextField;
	
	var BookPages: Array;
	var PageInfoA: Array;
	
	// This variable will store the page number to return to after a refresh.
	var iRestorePageOnComplete: Number = -1;
	var bNote: Boolean;

	var iCurrentLine: Number;
	var iLeftPageNumber: Number;
	var iMaxPageHeight: Number;
	var iNextPageBreak: Number;
	var iPageSetIndex: Number;
	var iPaginationIndex: Number;
	
	var ReferenceText_mc: TextField
	var ReferenceTextField: TextField;
	var RefTextFieldTextFormat: TextFormat;
	
	function BookMenu()
	{
		super();
		BookMenu.BookMenuInstance = this;
		BookPages = new Array();
		PageInfoA = new Array();
		iLeftPageNumber = 0;
		iPageSetIndex = 0;
		bNote = false;
		iRestorePageOnComplete = -1;
		ReferenceText_mc = ReferenceTextInstance;
		RefTextFieldTextFormat = ReferenceText_mc.PageTextField.getTextFormat();
        
	}
	
	function onLoad(): Void
	{
		ReferenceText_mc._visible = false;
		ReferenceTextField = ReferenceText_mc.PageTextField;

		ReferenceTextField.noTranslate = true;
		ReferenceTextField.setTextFormat(RefTextFieldTextFormat);
		iMaxPageHeight = ReferenceTextField._height;
		GameDelegate.addCallBack("SetBookText", this, "SetBookText");
		GameDelegate.addCallBack("TurnPage", this, "TurnPage");
		GameDelegate.addCallBack("PrepForClose", this, "PrepForClose");
        GameDelegate.addCallBack("GotoPageByAnchor", this, "GotoPageByAnchor");
        GameDelegate.addCallBack("SubmitTextInput", this, "SubmitTextInput");
		logToCpp("swf file loaded...")
	}

	function logToCpp(message:String):Void {
		skse.SendModEvent("DBF_DebugLogEvent", message, 0, 0);
	}

	function SubmitTextInput():Void
	{
		this.logToCpp("--- Logging Current Page Content ---");
		var foundPage:Boolean = false;

		// Find the current page's MovieClip in the cache
		for (var i:Number = 0; i < this.BookPages.length; ++i) {
			if (this.BookPages[i].pageNum == this.iLeftPageNumber) {
				var pageMC:MovieClip = this.BookPages[i];
				
				// Get both the plain text and the full HTML text
				var plainText:String = pageMC.PageTextField.text;
				var htmlText:String = pageMC.PageTextField.htmlText;

				// Send both to the C++ log
				this.logToCpp("Page " + this.iLeftPageNumber + " Plain Text: [" + plainText + "]");
				this.logToCpp("Page " + this.iLeftPageNumber + " HTML Text: [" + htmlText + "]");
				
				foundPage = true;
				break; 
			}
		}

		if (!foundPage) {
			this.logToCpp("Could not find a cached page MovieClip for page " + this.iLeftPageNumber);
		}
	}

	function SetBookInfo(a_formID:Number, a_title:String):Void
    {
        this.currentBookFormID = a_formID;
        this.sCurrentBookTitle = a_title;
    }

	function getCharRangeForPage(pageIndex:Number):Object {
		var charStart:Number = 0;
		var charEnd:Number = this.ReferenceTextField.text.length;

		var lineStart:Number = this.ReferenceTextField.getLineIndexAtPoint(0, this.PageInfoA[pageIndex].pageTop);
		if (lineStart != -1) {
			charStart = this.ReferenceTextField.getLineOffset(lineStart);
		}

		if (pageIndex + 1 < this.PageInfoA.length) {
			var lineEnd:Number = this.ReferenceTextField.getLineIndexAtPoint(0, this.PageInfoA[pageIndex + 1].pageTop);
			if (lineEnd != -1) {
				charEnd = this.ReferenceTextField.getLineOffset(lineEnd);
			}
		}

		return { start: charStart, end: charEnd };
	}
	
	function GotoPageByAnchor(a_anchorTag:String):Void
	{
		if (a_anchorTag == undefined || a_anchorTag == "") {
			return;
		}

		var targetPageIndex:Number = -1;
		
		for (var i:Number = 0; i < this.PageInfoA.length; ++i) {
			var pageText:String = this.ReferenceTextField.text.substring(this.getCharRangeForPage(i).start, this.getCharRangeForPage(i).end);
			var normalizedPageText:String = pageText.split("\r").join(" ").split("\n").join(" ");
			while (normalizedPageText.indexOf("  ") != -1) { 
				normalizedPageText = normalizedPageText.split("  ").join(" ");
			}
			if (normalizedPageText.indexOf(a_anchorTag) != -1) {
				targetPageIndex = i;
				break;
			}
		}
			
		
		if (targetPageIndex != -1) {
			var finalTargetPage:Number = targetPageIndex;
			// Ensure we land on the left-hand page of the spread
			if (finalTargetPage % 2 != 0) {
				finalTargetPage = finalTargetPage - 1;
			}
			//this.iPageSetIndex = Math.floor(finalTargetPage / BookMenu.CACHED_PAGES) * BookMenu.CACHED_PAGES;
			var delta:Number = finalTargetPage - this.iLeftPageNumber;
			this.TurnPage(delta);
			this.logToCpp("GotoPageByAnchor: Jump successful to spread starting with page " + finalTargetPage);
		} else {
			// Log an error if the anchor couldn't be found anywhere
			this.logToCpp("GotoPageByAnchor: ERROR - Could not find core anchor for '" + a_anchorTag + "'");
		}
	}

	
	function SetBookText(astrText: String, abNote: Boolean): Void
	{
		// --- FIX: Save the current page number before resetting everything ---
		iRestorePageOnComplete = iLeftPageNumber;

		resetState();
		
		bNote = abNote;
		ReferenceTextField.verticalAutoSize = "top";
		ReferenceTextField.SetText(astrText, true);
		if (abNote) 
			ReferenceTextField._width = BookMenu.NOTE_WIDTH;
		
		PageInfoA.push({pageTop: 0, pageHeight: iMaxPageHeight});
		iCurrentLine = 0;
		iPaginationIndex = setInterval(this, "CalculatePagination", 30);
		iNextPageBreak = iMaxPageHeight;
	}
	
	function CreateDisplayPage(PageTop: Number, PageBottom: Number, aPageNum: Number): Void
	{
		//this.logToCpp("CreateDisplayPage: Creating NEW page MovieClip for page number " + aPageNum);
		var Page_mc: MovieClip = ReferenceText_mc.duplicateMovieClip("Page", getNextHighestDepth());
		var PageTextField_tf: TextField = Page_mc.PageTextField;
		PageTextField_tf.noTranslate = true;
		PageTextField_tf.SetText(ReferenceTextField.htmlText, true);
		var iLineOffsetTop: Number = ReferenceTextField.getLineOffset(ReferenceTextField.getLineIndexAtPoint(0, PageTop));
		var iLineOffsetBottom: Number = ReferenceTextField.getLineOffset(ReferenceTextField.getLineIndexAtPoint(0, PageBottom));
		PageTextField_tf.replaceText(0, iLineOffsetTop, "");
		PageTextField_tf.replaceText(iLineOffsetBottom - iLineOffsetTop, ReferenceTextField.length, "");
		PageTextField_tf.autoSize = "left";
		if (bNote) {
			PageTextField_tf._width = BookMenu.NOTE_WIDTH;
			Page_mc._x = Stage.visibleRect.x + BookMenu.NOTE_X_OFFSET;
			Page_mc._y = Stage.visibleRect.y + BookMenu.NOTE_Y_OFFSET;
		} else {
			Page_mc._x = ReferenceText_mc._x;
			Page_mc._y = ReferenceText_mc._y;
		}
		var currentText:String = PageTextField_tf.text;
		var tagStart:Number = -1;

		// Loop as long as we can find a "[bookmark" tag
		while ((tagStart = currentText.indexOf("[bookmark")) > -1) {
			var tagEnd:Number = currentText.indexOf("]", tagStart);
			
			if (tagEnd > -1) {
				// CORRECT: Use the start and end indices to remove the tag
				PageTextField_tf.replaceText(tagStart, tagEnd + 1, "");
				
				// IMPORTANT: Refresh the 'currentText' variable because the text field
				// has been modified and all subsequent indices are now different.
				currentText = PageTextField_tf.text; 
			} else {
				// Failsafe: If an opening tag has no closing tag, exit the loop.
				break; 
			}
		}
		Page_mc._visible = false;
		Page_mc.pageNum = aPageNum;
		BookPages.push(Page_mc);
	}

	function CalculatePagination() {
		for (iCurrentLine; iCurrentLine <= ReferenceTextField.numLines; iCurrentLine++) {
			var iLineOffsetCurrent: Number = ReferenceTextField.getLineOffset(iCurrentLine);
			var iLineOffsetNext: Number = ReferenceTextField.getLineOffset(iCurrentLine + 1);
			var acharBoundaries: Object = ReferenceTextField.getCharBoundaries(iLineOffsetCurrent);
			var astrPageText: String = iLineOffsetNext == -1 ? ReferenceTextField.text.substring(iLineOffsetCurrent) : ReferenceTextField.text.substring(iLineOffsetCurrent, iLineOffsetNext);
			astrPageText = GlobalFunc.StringTrim(astrPageText);

			if (acharBoundaries.bottom > iNextPageBreak || astrPageText == BookMenu.PAGE_BREAK_TAG || iCurrentLine >= ReferenceTextField.numLines) {
				var aPageDims: Object = { pageTop: 0, pageHeight: iMaxPageHeight };

				if (astrPageText == BookMenu.PAGE_BREAK_TAG) {
					aPageDims.pageTop = acharBoundaries.bottom + ReferenceTextField.getLineMetrics(iCurrentLine).leading;
					PageInfoA[PageInfoA.length - 1].pageHeight = acharBoundaries.top - PageInfoA[PageInfoA.length - 1].pageTop;
				} else {
					aPageDims.pageTop = acharBoundaries.top;
					PageInfoA[PageInfoA.length - 1].pageHeight = aPageDims.pageTop - PageInfoA[PageInfoA.length - 1].pageTop;
				}

				iNextPageBreak = aPageDims.pageTop + iMaxPageHeight;
				PageInfoA.push(aPageDims);
			}
		}

		// Pagination done
		if (iCurrentLine >= ReferenceTextField.numLines) {
			clearInterval(iPaginationIndex);
			iPaginationIndex = -1;

			// --- FIX: Restore the correct page index after pagination is complete ---
			if (iRestorePageOnComplete >= 0 && iRestorePageOnComplete < PageInfoA.length) {
				SetLeftPageNumber(iRestorePageOnComplete);
				iPageSetIndex = iRestorePageOnComplete;
			} else {
				SetLeftPageNumber(0);
				iPageSetIndex = 0;
			}
			// Reset the restore flag
			iRestorePageOnComplete = -1;

			// Clear any old pages and update the display
			for (var i = 0; i < BookPages.length; i++) {
				BookPages[i].removeMovieClip();
			}
			BookPages = [];
			UpdatePages();
		}
	}

	function SetLeftPageNumber(aiPageNum: Number): Void
	{
		// The added braces ensure the page number is ONLY set if it's valid.
		if (aiPageNum < this.PageInfoA.length) {
			this.logToCpp("SetLeftPageNumber: Setting iLeftPageNumber to " + aiPageNum);
			this.iLeftPageNumber = aiPageNum;
		} else {
			this.logToCpp("SetLeftPageNumber: REJECTED attempt to set invalid page number: " + aiPageNum);
		}
	}

	function ShowPageAtOffset(aiPageOffset: Number): Void
	{
		for (var i: Number = 0; i < BookPages.length; i++)
			if (BookPages[i].pageNum == iPageSetIndex + aiPageOffset) 
				BookPages[i]._visible = true;
			else
				BookPages[i]._visible = false;
	}
	
	function PrepForClose(): Void
	{
		iPageSetIndex = iLeftPageNumber;
	}
	
	
	function TurnPage(aiDelta: Number): Boolean
	{		
		var iNewPageNumber: Number = iLeftPageNumber + aiDelta;
		var bValidTurn: Boolean = iNewPageNumber >= 0 && iNewPageNumber < PageInfoA.length;
		if (bNote) 
			bValidTurn = iNewPageNumber >= 0 && iNewPageNumber < PageInfoA.length - 1;
		
		var iPagestoTurn: Number = Math.abs(aiDelta);
		if (bValidTurn) {
			var iMaxTurnablePages: Number = iPagestoTurn == 1 ? 1 : 4;
			SetLeftPageNumber(iNewPageNumber);
			if (iLeftPageNumber < iPageSetIndex) 
				iPageSetIndex = iPageSetIndex - iPagestoTurn;
			else if (iLeftPageNumber >= iPageSetIndex + iMaxTurnablePages) 
				iPageSetIndex = iPageSetIndex + iPagestoTurn;
			UpdatePages();
		}
		return bValidTurn;
	}
	
	function UpdatePages(): Void
	{
		this.logToCpp("UpdatePages: Running with iPageSetIndex = " + this.iPageSetIndex + " and cache size = " + this.BookPages.length);
		for (var i: Number = 0; i < BookMenu.CACHED_PAGES; i++) {
			var bCachedPage: Boolean = false;
			for (var j = 0; j < BookPages.length && !bCachedPage; j++)
				if (BookPages[j].pageNum == iPageSetIndex + i) 
					bCachedPage = true;
			
			if (!bCachedPage && (PageInfoA.length > iPageSetIndex + i + 1 || (iPaginationIndex == -1 && PageInfoA.length > iPageSetIndex + i))) 
				CreateDisplayPage(PageInfoA[iPageSetIndex + i].pageTop, PageInfoA[iPageSetIndex + i].pageTop + PageInfoA[iPageSetIndex + i].pageHeight, iPageSetIndex + i);
		}
		
		for (var i: Number = BookPages.length - 1; i >= 0; i--) {
			if (BookPages[i].pageNum < iPageSetIndex || BookPages[i].pageNum >= iPageSetIndex + BookMenu.CACHED_PAGES) {
				BookPages[i].removeMovieClip();
				BookPages.splice(i, 1);
			}
		}
	}

function resetState()
	{
		for (var i:Number = 0; i < BookPages.length; i++) {
			BookPages[i].removeMovieClip();
		}
		BookPages = new Array();
		PageInfoA = new Array();

		if (ReferenceTextField != undefined) {
			ReferenceTextField.text = "";
		}

		if (iPaginationIndex != -1) {
			clearInterval(iPaginationIndex);
			iPaginationIndex = -1;
		}

		// Reset to clean defaults
		iLeftPageNumber = 0;
		iPageSetIndex = 0;
	}
}