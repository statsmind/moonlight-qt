using System;
using System.Net.Http;
using System.Text;
using System.Text.Json.Serialization;
using System.Text.Json;
using System.Threading.Tasks;

namespace Moonlight
{
    /**
     * 模拟云游戏客户端
     * 测试 https://cloudgame-test.mingland.cn/prod-api/
     * 生产 https://cloudgame.tdsmartcloud.com:8081/prod-api
     * 上海 https://cg.wostore.cn/cloud-game-app-api
     * 山西 https://cloudgame.tdsmartcloud.com:8081/cloud-game-app-api
     */
    public class CloudGameClient
    {
        private const string BASE_URL = "https://cloudgame-test.mingland.cn/prod-api";

        private const string CHANNEL_ID = "pang_bao";

        private HttpClient httpClient = new HttpClient();
            
        private JsonSerializerOptions JsonOptions = new JsonSerializerOptions
        {
            PropertyNameCaseInsensitive = true,
            // 设置默认命名策略为 camelCase，与 JSON 属性名匹配
            PropertyNamingPolicy = JsonNamingPolicy.CamelCase,
            // 允许尾随逗号
            AllowTrailingCommas = true,
            // 处理空值
            DefaultIgnoreCondition = JsonIgnoreCondition.WhenWritingNull,
            RespectNullableAnnotations = true,
        };

        private async Task<T> SendAsync<T>(HttpRequestMessage request)
        {
            var response = await httpClient.SendAsync(request);
            string responseContent = await response.Content.ReadAsStringAsync();

            var httpResponse = JsonSerializer.Deserialize<HttpResponse<T>>(responseContent, JsonOptions);
            if (httpResponse.Code != 200)
            {
                throw new Exception(httpResponse.Msg);
            }

            return httpResponse.Data;
        }

        public async Task<DeviceInfo> RegisterDevice(string deviceCode)
        {
            // 将对象序列化为 JSON 字符串
            string jsonData = JsonSerializer.Serialize(new
            {
                deviceCode = "jameshu_test",
                model = "win10",
                type = "5"
            });

            var request = new HttpRequestMessage(HttpMethod.Post, $"{BASE_URL}/business/container/device/register")
            {
                Content = new StringContent(jsonData, Encoding.UTF8, "application/json")
            };
            request.Headers.Add("channelId", CHANNEL_ID);

            return await SendAsync<DeviceInfo>(request);
        }

        public async Task<QrCode> GetQrcode(DeviceInfo deviceInfo)
        {
            var request = new HttpRequestMessage(HttpMethod.Get, $"{BASE_URL}/business/devices/qrcode?deviceId={deviceInfo.DeviceId}");
            request.Headers.Add("channelId", CHANNEL_ID);

            return await SendAsync<QrCode>(request);
        }

        public async Task<FreeWindow> QueryFreeWindows(string gameId, string userId, string gpuId = "4060")
        {
            var requestData = new
            {
                gameId = gameId,
                gpuId = gpuId,
                token = userId
            };

            string jsonData = JsonSerializer.Serialize(requestData);

            var request = new HttpRequestMessage(HttpMethod.Post, $"{BASE_URL}/business/cloudGameApp/queryFreeWindows")
            {
                Content = new StringContent(jsonData, Encoding.UTF8, "application/json")
            };
            request.Headers.Add("channelId", CHANNEL_ID);

            return await SendAsync<FreeWindow>(request);
        }

        static void ProcessSSEData(string jsonData)
        {
            try
            {
                // 这里应该根据实际的JSON格式解析数据
                // 示例：假设数据包含状态信息
                using (var document = JsonDocument.Parse(jsonData))
                {
                    var root = document.RootElement;

                    // 检查是否存在特定字段，如status、eventType等
                    if (root.TryGetProperty("status", out var statusElement))
                    {
                        string status = statusElement.GetString();
                        if (status == "paired" || status == "connected")
                        {
                            Console.WriteLine("设备已配对或连接成功");
                            // 在此处可以触发下一步操作
                        }
                    }

                    // 处理其他可能的事件类型
                    if (root.TryGetProperty("eventType", out var eventTypeElement))
                    {
                        string eventType = eventTypeElement.GetString();
                        switch (eventType)
                        {
                            case "device_paired":
                                Console.WriteLine("收到设备配对事件");
                                // 可以在这里启动Sunshine配对流程
                                // PairWithSunshine(ipAddress, pin);
                                break;
                            case "connection_established":
                                Console.WriteLine("连接已建立");
                                break;
                            case "session_started":
                                Console.WriteLine("会话已开始");
                                break;
                            default:
                                Console.WriteLine($"收到未知事件类型: {eventType}");
                                break;
                        }
                    }
                }
            }
            catch (JsonException ex)
            {
                Console.WriteLine($"解析JSON数据时出错: {ex.Message}");
            }
        }
    }
}