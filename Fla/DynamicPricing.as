import skse;

class DynamicPricing extends MovieClip {

    public static var instance;

    /* refs */
    public var Menu:MovieClip;
    public var inventoryLists:MovieClip;
    public var itemList:MovieClip;
    public var ItemInfo:MovieClip;
    public var processor:Object;

    /* config */
    public var bIndicators:Boolean = true;
    public var bPriceColorCoding:Boolean = true;
    public var favorablePriceColor:Number = 0x00ff00;
    public var unfavorablePriceColor = 0xff0000;
    public var indicatorPadding:Number = 5;

    public var data:Object;
    private var favorablesSells:Object;
    private var favorablesBuys:Object;
    private var PriceTfName:String; /* name of TextField in entryClip that holds price value */
    private var lastHighlightIndex:Number = -1;
    private var indicators:Array = new Array();

    function DynamicPricing() {
        DynamicPricing.instance = this;
        data = _root.DPF;
        var config = _root.DPF_Config;
        if (config !== undefined) {
            bIndicators = config.showIndicators;
            bPriceColorCoding = config.colorCode;
            favorablePriceColor = parseInt(config.favorablePriceColor, 16);
            unfavorablePriceColor = parseInt(config.unfavorablePriceColor, 16);
            indicatorPadding = config.indicatorPadding;
        }
    }

    function onLoad() {
        Menu = _parent._parent.Menu_mc;
        inventoryLists = Menu.inventoryLists;
        itemList = inventoryLists.itemList; /* skyui.components.list.TabularList */
        favorablesSells = new Object();
        favorablesBuys = new Object();

        if (itemList._dataProcessors.length === 0) {
            // we're early, wait for setConfig call, then override
            duckPunchSetConfig();
        } else {
            // we're in, update
            duckPunch();
        }
        setupEvents();
    }

    function setupEvents() {
        if (bPriceColorCoding) {
            itemList.addEventListener("selectionChange", this, "onItemHighlightChange");
        }
    }

    // setEntry resets tf.textColor, so it needs to be reapplied
    private function onItemHighlightChange(event: Object): Void {
        if (lastHighlightIndex !== -1) {
            maybeAddIndicatorForEntry(itemList._entryList[lastHighlightIndex], Menu.isViewingVendorItems());
        }
        maybeAddIndicatorForEntry(itemList.selectedEntry, Menu.isViewingVendorItems());
        lastHighlightIndex = event.index;
    }

    function duckPunchSetConfig() {
        Menu.DPF__setConfig = Menu.setConfig;
        Menu.setConfig = setConfig;
    }

    function duckPunch() {
        Menu.DPF__UpdateItemCardInfo = Menu.UpdateItemCardInfo;
        Menu.UpdateItemCardInfo = UpdateItemCardInfo;

        processor = itemList._dataProcessors[0];
        processor.DPF__processEntry = processor.processEntry;
        processor.processEntry = processEntry;

        if (bPriceColorCoding || bIndicators) {
            itemList.DPF__UpdateList = itemList.UpdateList;
            itemList.UpdateList = UpdateList;
        }
    }

    // override Menu.setConfig
    function setConfig(a_config: Object): Void {
        this = DynamicPricing.instance;
        Menu.DPF__setConfig(a_config);
        duckPunch();
    }

    public function UpdateList(): Void {
        this = DynamicPricing.instance;
        itemList.DPF__UpdateList();
        updatePriceTfName();
        clearExistingTags();

        if (inventoryLists.currentState !== 1) { // 1: SHOW_PANEL
            // inventoryLists is animating, wait for it to finish
            onEnterFrame = function() {
                if (inventoryLists.currentState !== 1) return;
                addIndicators();
                onEnterFrame = null;
            }
        } else {
            addIndicators();
        }
    }

    function addIndicators() {
        var _listIndex = 0,
            isBuying = Menu.isViewingVendorItems();
        for (var i = itemList._scrollPosition; i < itemList.getListEnumSize() && _listIndex < itemList._maxListIndex; i++) {
            var entryItem:Object = itemList.getListEnumEntry(i);
            maybeAddIndicatorForEntry(entryItem, isBuying);
            ++_listIndex;
        }
    }

    function getAbsolutePosition(targetObject:Object) : Object {
        var points = {
            x : targetObject._x,
            y : targetObject._y
        };
        targetObject._parent.localToGlobal(points);

        return points;
    }

    function clearExistingTags() {
        for ( var i = 0; i < indicators.length; i++ ) {
            indicators[i].removeMovieClip();
        }
        indicators = new Array();
    }

    // hacky way to find which textField in entryClip is showing price
    function updatePriceTfName() {
        var layout = itemList.layout;
        for (var i = 0; i < layout.columnCount; i++) {
            var columnData = layout.columnLayoutData[i];
            if (columnData.entryValue === '@infoValue') {
                PriceTfName = columnData.stageName;
                break;
            }
        }
    }
    
    function maybeAddIndicatorForEntry(entryItem:Object, isBuying:Boolean) {
        var favorable:Boolean = isBuying ? favorablesBuys[entryItem.formId] : favorablesSells[entryItem.formId];
        if (favorable !== undefined) {
            var entryClip = itemList.getClipByIndex(entryItem.clipIndex),
                tf = entryClip[PriceTfName];
            if (bPriceColorCoding) {
                tf.textColor = favorable === true ? favorablePriceColor : unfavorablePriceColor;
            }

            if (bIndicators) {
                var indicatorName:String =
                    isBuying && favorable === true ? 'BuyDown'
                    : isBuying ? 'BuyUp'
                    : favorable ? 'SellUp'
                    : 'SellDown';
                var pos:Object = getAbsolutePosition(tf),
                    mc:MovieClip = attachMovie(indicatorName, 'dpf_' + entryItem.clipIndex, getNextHighestDepth());
                mc._x = pos.x + tf._width + indicatorPadding;
                mc._y = pos.y + (tf._height / 2 - mc._height / 2);
                indicators.push(mc);
            }
        }
    }

    public function processEntry(a_entryObject: Object, a_itemInfo: Object): Void {
        this = DynamicPricing.instance;
        var isBuying = a_entryObject.filterFlag > 1024,
            result:Array = processItem(a_entryObject.keywords, a_entryObject.formId, a_itemInfo.value, isBuying);
        a_itemInfo.value = Math.floor(result[0]);

        if (result[1]) {
            // call BarterDataSetter.processEntry which applies default barter multipliers
            processor.DPF__processEntry(a_entryObject, a_itemInfo);
        } else {
            // skip BarterDataSetter.processEntry
            InventoryDataSetter.prototype.processEntry.call(processor, a_entryObject, a_itemInfo);
        }
    }

    // override Menu.UpdateItemCardInfo
    function UpdateItemCardInfo(a_updateObj: Object): Void {
        this = DynamicPricing.instance;
        var result:Array = processItem(itemList.selectedEntry.keywords, a_updateObj.formId, a_updateObj.value, Menu.isViewingVendorItems());
        a_updateObj.value = Math.floor(result[0]);
        if (result[1]) {
            Menu.DPF__UpdateItemCardInfo(a_updateObj);
        } else {
            // stripped version of Menu.UpdateItemCardInfo which skips applying default barter multipliers
            Menu.itemCard.itemInfo = a_updateObj;
            Menu.bottomBar.updateBarterPerItemInfo(a_updateObj);
        }
    }

    function processItem(itemKeywords:Object, formId:Number, price:Number, isBuying:Boolean) : Array {
        var total:Number = 1;
        var defaultMults:Boolean = true;

        for (var i = 0; i < data.length; i++) {
            if (doKeywordsMatch(data[i].keywords, itemKeywords)) {
                if (data[i].defaultMults === false) defaultMults = false;
                var mult:Number = isBuying ? data[i].buy : data[i].sell;
                total += (mult - 1);
            }
        }
        if (total < 0) total = 0; /* in game, buy prices never drop below 1 */

        if (total !== 1 && (bPriceColorCoding || bIndicators)) {
            if (isBuying) {
                favorablesBuys[formId] = total < 1;
            } else {
                favorablesSells[formId] = total > 1;
            }
        }

        return [
            price * total,
            defaultMults
        ];
    }

    // check if an item matches any of the keywords specified in the rule
    function doKeywordsMatch(ruleKeywords:Array, itemKeywords:Object) : Boolean {
        if (ruleKeywords.length) {
            for (var i = 0; i < ruleKeywords.length; i++) {
                if ( itemKeywords[ruleKeywords[i]] === true ) {
                    return true;
                }
            }
            return false;
        } else {
            // no keyword is specified, applies to all
            return true;
        }
    }

    function LogObject( obj ) {
        var s = '';
        for ( var i in obj ) {
            s += i + ': ' + obj[i] + ';\n';
        }
        skse.Log(s);
    }
}