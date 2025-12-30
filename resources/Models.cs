using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Text.Json.Serialization;
using System.Threading.Tasks;



namespace Moonlight
{
    public class HttpResponse<T>
    {
        [JsonPropertyName("code")]
        public int Code { get; set; }

        [JsonPropertyName("msg")]
        public string Msg { get; set; }

        [JsonPropertyName("data")]
        public T Data { get; set; }
    }

    public class QrCode
    {
        /**
         * Base64编码的二维码图片
         */
        [JsonPropertyName("qrCodeUrl")]
        public string QrCodeUrl { get; set; }

        [JsonPropertyName("qrCodeType")]
        public string QrCodeType { get; set; }

        [JsonPropertyName("isBind")]
        public bool? IsBind { get; set; }
    }

    public class DeviceInfo
    {
        [JsonPropertyName("deviceId")]
        public string DeviceId { get; set; }

        [JsonPropertyName("name")]
        [JsonIgnore(Condition = JsonIgnoreCondition.WhenWritingNull)]
        public string Name { get; set; }

        [JsonPropertyName("qrCodeUrl")]
        [JsonIgnore(Condition = JsonIgnoreCondition.WhenWritingNull)]
        public string QrCodeUrl { get; set; }

        [JsonPropertyName("isBind")]
        public bool? IsBind { get; set; }

        [JsonPropertyName("skuCustomPrice")]
        public List<SkuCustomPrice> SkuCustomPrice { get; set; }

        [JsonPropertyName("userInfo")]
        [JsonIgnore(Condition = JsonIgnoreCondition.WhenWritingNull)]
        public UserInfo UserInfo { get; set; }

        [JsonPropertyName("gameList")]
        [JsonIgnore(Condition = JsonIgnoreCondition.WhenWritingNull)]
        public List<Game> GameList { get; set; }

        [JsonPropertyName("gameCategoryList")]
        public List<GameCategory> GameCategoryList { get; set; }

        [JsonPropertyName("tenantId")]
        [JsonIgnore(Condition = JsonIgnoreCondition.WhenWritingNull)]
        public object TenantId { get; set; }

        [JsonPropertyName("vol")]
        public int? Vol { get; set; }

        [JsonPropertyName("useType")]
        [JsonIgnore(Condition = JsonIgnoreCondition.WhenWritingNull)]
        public object UseType { get; set; }

        [JsonPropertyName("canRefund")]
        public bool? CanRefund { get; set; }
    }

    public class SkuCustomPrice
    {
        [JsonPropertyName("id")]
        [JsonIgnore(Condition = JsonIgnoreCondition.WhenWritingNull)]
        public object Id { get; set; }

        [JsonPropertyName("skuId")]
        public int? SkuId { get; set; }

        [JsonPropertyName("deviceCode")]
        public string DeviceCode { get; set; }

        [JsonPropertyName("deviceName")]
        [JsonIgnore(Condition = JsonIgnoreCondition.WhenWritingNull)]
        public object DeviceName { get; set; }

        [JsonPropertyName("newSaleMoney")]
        public int? NewSaleMoney { get; set; }

        [JsonPropertyName("isHot")]
        [JsonIgnore(Condition = JsonIgnoreCondition.WhenWritingNull)]
        public object IsHot { get; set; }

        [JsonPropertyName("skuCode")]
        public string SkuCode { get; set; }

        [JsonPropertyName("skuName")]
        public string SkuName { get; set; }

        [JsonPropertyName("skuShortName")]
        public string SkuShortName { get; set; }

        [JsonPropertyName("description")]
        public string Description { get; set; }

        [JsonPropertyName("pictureUrl")]
        [JsonIgnore(Condition = JsonIgnoreCondition.WhenWritingNull)]
        public object PictureUrl { get; set; }

        [JsonPropertyName("skuTypeCode")]
        public string SkuTypeCode { get; set; }

        [JsonPropertyName("skuTypeName")]
        public string SkuTypeName { get; set; }

        [JsonPropertyName("displayResolution")]
        public string DisplayResolution { get; set; }

        [JsonPropertyName("baseSaleMoney")]
        public int? BaseSaleMoney { get; set; }

        [JsonPropertyName("cycleDuration")]
        public int? CycleDuration { get; set; }

        [JsonPropertyName("status")]
        public int? Status { get; set; }
    }

    /**
     * ValueKind = Object : "{"userId":"1905140397582258178","deptId":null,"userName":"otjsA7NSZOfGsmi5Bo6LxzG7LEbs","nickName":"用户08460128","userType":"xcx_user","email":"","phonenumber":"13910022763","sex":"0","status":"0","createTime":"2025-03-27 14:10:41","merchant":"1","openId":"omvRLvgbf4g2XXC4lWu2WAicjGFk","avatar":"","orderNum":6,"buyTime":null,"skuType":null,"orderMoney":null,"newUser":false,"goldCoin":0,"preCoin":0,"baseCoin":0,"giftCoin":0,"profit":0,"useType":1,"vipDate":"2025-12-10","vip":false,"vipTodayBalance":null,"skuList":[{"skuTypeCode":"4060","timeBalance":0}],"channelId":null,"newUserGiftCoin":60,"giftCoinFlag":false,"authInstall":false}"
     **/

    public class UserInfo
    {
        [JsonPropertyName("userId")]
        public string UserId { get; set; }

        [JsonPropertyName("deptId")]
        [JsonIgnore(Condition = JsonIgnoreCondition.WhenWritingNull)]
        public object DeptId { get; set; }

        [JsonPropertyName("userName")]
        public string UserName { get; set; }

        [JsonPropertyName("nickName")]
        public string NickName { get; set; }

        [JsonPropertyName("userType")]
        public string UserType { get; set; }

        [JsonPropertyName("email")]
        public string Email { get; set; }

        [JsonPropertyName("phonenumber")]
        public string Phonenumber { get; set; }

        [JsonPropertyName("sex")]
        public string Sex { get; set; }

        [JsonPropertyName("status")]
        public string Status { get; set; }

        [JsonPropertyName("createTime")]
        public string CreateTime { get; set; }

        [JsonPropertyName("merchant")]
        public string Merchant { get; set; }

        [JsonPropertyName("openId")]
        public string OpenId { get; set; }

        [JsonPropertyName("avatar")]
        public string Avatar { get; set; }

        [JsonPropertyName("orderNum")]
        public int? OrderNum { get; set; }

        [JsonPropertyName("buyTime")]
        [JsonIgnore(Condition = JsonIgnoreCondition.WhenWritingNull)]
        public object BuyTime { get; set; }

        [JsonPropertyName("skuType")]
        [JsonIgnore(Condition = JsonIgnoreCondition.WhenWritingNull)]
        public object SkuType { get; set; }

        [JsonPropertyName("orderMoney")]
        [JsonIgnore(Condition = JsonIgnoreCondition.WhenWritingNull)]
        public object OrderMoney { get; set; }

        [JsonPropertyName("newUser")]
        public bool? NewUser { get; set; }

        [JsonPropertyName("goldCoin")]
        public int? GoldCoin { get; set; }

        [JsonPropertyName("preCoin")]
        public int? PreCoin { get; set; }

        [JsonPropertyName("baseCoin")]
        public int? BaseCoin { get; set; }

        [JsonPropertyName("giftCoin")]
        public int? GiftCoin { get; set; }

        [JsonPropertyName("profit")]
        public int? Profit { get; set; }

        [JsonPropertyName("useType")]
        public int? UseType { get; set; }

        [JsonPropertyName("vipDate")]
        public string VipDate { get; set; }

        [JsonPropertyName("vip")]
        public bool? Vip { get; set; }

        [JsonPropertyName("vipTodayBalance")]
        [JsonIgnore(Condition = JsonIgnoreCondition.WhenWritingNull)]
        public object VipTodayBalance { get; set; }

        [JsonPropertyName("skuList")]
        public List<Sku> SkuList { get; set; }

        [JsonPropertyName("channelId")]
        [JsonIgnore(Condition = JsonIgnoreCondition.WhenWritingNull)]
        public object ChannelId { get; set; }

        [JsonPropertyName("newUserGiftCoin")]
        public int? NewUserGiftCoin { get; set; }

        [JsonPropertyName("giftCoinFlag")]
        public bool? GiftCoinFlag { get; set; }

        [JsonPropertyName("authInstall")]
        public bool? AuthInstall { get; set; }
    }

    public class Sku
    {
        [JsonPropertyName("skuTypeCode")]
        public string SkuTypeCode { get; set; }

        [JsonPropertyName("timeBalance")]
        public int? TimeBalance { get; set; }
    }

    public class GameCategory
    {
        [JsonPropertyName("id")]
        public string Id { get; set; }

        [JsonPropertyName("name")]
        public string Name { get; set; }

        [JsonPropertyName("gameList")]
        public List<Game> GameList { get; set; }
    }

    public class Game
    {
        [JsonPropertyName("id")]
        public object Id { get; set; }

        [JsonPropertyName("gameId")]
        public int? GameId { get; set; }

        [JsonPropertyName("gameName")]
        public string GameName { get; set; }

        [JsonPropertyName("gameTypeList")]
        public List<int> GameTypeList { get; set; }

        [JsonPropertyName("details")]
        public string Details { get; set; }

        [JsonPropertyName("installPath")]
        [JsonIgnore(Condition = JsonIgnoreCondition.WhenWritingNull)]
        public string InstallPath { get; set; }

        [JsonPropertyName("gameCapacity")]
        public string GameCapacity { get; set; }

        [JsonPropertyName("gameIssuer")]
        public string GameIssuer { get; set; }

        [JsonPropertyName("issueTime")]
        public string IssueTime { get; set; }

        [JsonPropertyName("language")]
        public int? Language { get; set; }

        [JsonPropertyName("archivePath")]
        public string ArchivePath { get; set; }

        [JsonPropertyName("archiveDir")]
        [JsonIgnore(Condition = JsonIgnoreCondition.WhenWritingNull)]
        public object ArchiveDir { get; set; }

        [JsonPropertyName("archiveFile")]
        [JsonIgnore(Condition = JsonIgnoreCondition.WhenWritingNull)]
        public object ArchiveFile { get; set; }

        [JsonPropertyName("archiveEnable")]
        public int? ArchiveEnable { get; set; }

        [JsonPropertyName("isHandShank")]
        public int? IsHandShank { get; set; }

        [JsonPropertyName("windowName")]
        [JsonIgnore(Condition = JsonIgnoreCondition.WhenWritingNull)]
        public string WindowName { get; set; }

        [JsonPropertyName("kernelProcess")]
        public string KernelProcess { get; set; }

        [JsonPropertyName("wallpaper")]
        public string Wallpaper { get; set; }

        [JsonPropertyName("gameVideo")]
        public string GameVideo { get; set; }

        [JsonPropertyName("icon")]
        public string Icon { get; set; }

        [JsonPropertyName("isOnline")]
        public int? IsOnline { get; set; }

        [JsonPropertyName("exclusiveEnable")]
        public int? ExclusiveEnable { get; set; }

        [JsonPropertyName("tags")]
        public List<Tag> Tags { get; set; }

        [JsonPropertyName("loadingImg")]
        public string LoadingImg { get; set; }

        [JsonPropertyName("strategyImageList")]
        public List<string> StrategyImageList { get; set; }

        [JsonPropertyName("sortOrder")]
        public int? SortOrder { get; set; }
    }

    public class Tag
    {
        [JsonPropertyName("id")]
        public string Id { get; set; }

        [JsonPropertyName("name")]
        public string Name { get; set; }
    }

    public class PortGroupItem
    {
        [JsonPropertyName("id")]
        public int? Id { get; set; }

        [JsonPropertyName("portGroupName")]
        public string PortGroupName { get; set; }

        [JsonPropertyName("internalPort")]
        public int? InternalPort { get; set; }

        [JsonPropertyName("forwardPort")]
        public int? ForwardPort { get; set; }
    }

    public class CloudGameVo
    {
        // 根据实际情况添加属性，JSON中为null
    }

    public class FreeWindow
    {
        [JsonPropertyName("deviceIp")]
        public string DeviceIp { get; set; }

        [JsonPropertyName("deviceId")]
        public string DeviceId { get; set; }

        [JsonPropertyName("token")]
        public string Token { get; set; }

        [JsonPropertyName("portGroupList")]
        public List<PortGroupItem> PortGroupList { get; set; }

        [JsonPropertyName("archivePath")]
        public string ArchivePath { get; set; }

        [JsonPropertyName("cloudGameVo")]
        public object CloudGameVo { get; set; }

        [JsonPropertyName("portGroup")]
        [JsonIgnore(Condition = JsonIgnoreCondition.WhenWritingNull)]
        public object PortGroup { get; set; } // JSON中为null，使用object或具体类型

        [JsonPropertyName("uuuid")]
        public string Uuid { get; set; }

        [JsonPropertyName("sslEnable")]
        public bool? SslEnable { get; set; }
    }
}
