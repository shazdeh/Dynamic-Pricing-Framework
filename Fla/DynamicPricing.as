import skse;

class DynamicPricing extends MovieClip {

    public static var instance;

    public var Menu:MovieClip;
    public var inventoryLists:MovieClip;
    public var itemList:MovieClip;
    public var ItemInfo:MovieClip;
    public var processor:Object;

    public var data:Object;

    function DynamicPricing() {
        DynamicPricing.instance = this;
        data = _root.DPF;
    }

    function onLoad() {
        Menu = _parent._parent.Menu_mc;
        inventoryLists = Menu.inventoryLists;
        itemList = inventoryLists.itemList;

        if (itemList._dataProcessors.length === 0) {
            // we're early, wait for setConfig call, then override
            duckPunchSetConfig();
        } else {
            // we're in, update
            duckPunch();
        }
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
    }

    // override Menu.setConfig
    function setConfig(a_config: Object): Void {
        this = DynamicPricing.instance;
        Menu.DPF__setConfig(a_config);
        duckPunch();
    }

    public function processEntry(a_entryObject: Object, a_itemInfo: Object): Void {
        this = DynamicPricing.instance;
        a_itemInfo.value = Math.floor(processItem(a_entryObject.keywords, a_itemInfo.value, a_entryObject.filterFlag > 1024));
        processor.DPF__processEntry(a_entryObject, a_itemInfo);
    }

    // override Menu.UpdateItemCardInfo
    function UpdateItemCardInfo(a_updateObj: Object): Void {
        this = DynamicPricing.instance;
        a_updateObj.value = Math.floor(processItem(itemList.selectedEntry.keywords, a_updateObj.value, Menu.isViewingVendorItems()));
        Menu.DPF__UpdateItemCardInfo(a_updateObj);
    }

    function processItem(itemKeywords:Object, price:Number, isBuying:Boolean) : Number {
        for (var i = 0; i < data.length; i++) {
            if (doKeywordsMatch(data[i].keywords, itemKeywords)) {
                price *= isBuying ? data[i].buy : data[i].sell;
            }
        }

        return price;
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