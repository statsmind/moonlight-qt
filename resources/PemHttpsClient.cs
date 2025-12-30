using System;
using System.IO;
using System.Net.Http;
using System.Net.Security;
using System.Security.Authentication;
using System.Security.Cryptography.X509Certificates;
using System.Text;
using System.Threading.Tasks;
using Microsoft.Extensions.Logging;
using Microsoft.Extensions.Logging.Abstractions;
using Org.BouncyCastle.Crypto;
using Org.BouncyCastle.Crypto.Parameters;
using Org.BouncyCastle.OpenSsl;
using Org.BouncyCastle.Security;
using Org.BouncyCastle.X509;

namespace Example
{
    public class PemHttpsClientWithBouncyCastle : IDisposable
    {
        private readonly ILogger<PemHttpsClientWithBouncyCastle> _logger;
        private readonly HttpClient _httpClient;
        private readonly HttpClientHandler _httpClientHandler;
        private readonly X509Certificate2 _clientCertificate;
        private readonly string _baseUrl;

        public PemHttpsClientWithBouncyCastle(string host, int port, string basePath = "/")
        {
            _logger = NullLogger<PemHttpsClientWithBouncyCastle>.Instance;
            _baseUrl = $"https://{host}:{port}{basePath.TrimEnd('/')}/";

            _logger.LogInformation("正在配置HTTPS客户端，目标URL: {BaseUrl}", _baseUrl);

            _clientCertificate = LoadCertificateWithBouncyCastle();
            _httpClientHandler = CreateHttpClientHandler(_clientCertificate);
            _httpClient = new HttpClient(_httpClientHandler)
            {
                Timeout = TimeSpan.FromSeconds(30)
            };
        }

        public X509Certificate2 ClientCertificate => _clientCertificate;

        private HttpClientHandler CreateHttpClientHandler(X509Certificate2 clientCertificate)
        {
            try
            {
                // 使用 BouncyCastle 加载证书
                //clientCertificate = LoadCertificateWithBouncyCastle();

                var handler = new HttpClientHandler
                {
                    ClientCertificateOptions = ClientCertificateOption.Manual,
                    SslProtocols = SslProtocols.Tls12,
                    ServerCertificateCustomValidationCallback = (request, cert, chain, errors) =>
                    {
                        // 对于IP地址，跳过主机名验证
                        _logger.LogInformation("验证服务器证书，主机: {Host}", request?.RequestUri?.Host);
                        return true;
                    }
                };

                handler.ClientCertificates.Add(clientCertificate);

                _logger.LogInformation("HTTPS客户端配置完成");
                return handler;
            }
            catch (Exception ex)
            {
                _logger.LogError(ex, "创建HttpClientHandler时发生错误");
                throw;
            }
        }

        private X509Certificate2 LoadCertificateWithBouncyCastle()
        {
            const string clientCertPath = "Resources/fullchain.crt";
            const string clientKeyPath = "Resources/client.key";

            _logger.LogInformation("正在加载证书: {CertPath}", clientCertPath);
            _logger.LogInformation("正在加载私钥: {KeyPath}", clientKeyPath);

            try
            {
                // 加载证书
                X509Certificate2 certificate;
                using (var reader = new StreamReader(clientCertPath))
                {
                    var pemReader = new PemReader(reader);
                    var certObject = pemReader.ReadObject();

                    if (certObject is Org.BouncyCastle.X509.X509Certificate bcCert)
                    {
                        certificate = new X509Certificate2(bcCert.GetEncoded());
                    }
                    else
                    {
                        throw new InvalidDataException("无效的证书格式");
                    }
                }

                // 加载私钥
                AsymmetricKeyParameter privateKey;
                using (var reader = new StreamReader(clientKeyPath))
                {
                    var pemReader = new PemReader(reader);
                    var keyObject = pemReader.ReadObject();

                    if (keyObject is AsymmetricCipherKeyPair keyPair)
                    {
                        privateKey = keyPair.Private;
                    }
                    else if (keyObject is AsymmetricKeyParameter akp)
                    {
                        privateKey = akp;
                    }
                    else
                    {
                        throw new InvalidDataException("无效的私钥格式");
                    }
                }

                // 合并证书和私钥
                if (privateKey is RsaPrivateCrtKeyParameters rsaPrivate)
                {
                    var rsaParameters = DotNetUtilities.ToRSAParameters(rsaPrivate);
                    using (var rsa = System.Security.Cryptography.RSA.Create())
                    {
                        rsa.ImportParameters(rsaParameters);
                        var certWithKey = certificate.CopyWithPrivateKey(rsa);
                        return new X509Certificate2(certWithKey.Export(X509ContentType.Pfx));
                    }
                }
                else
                {
                    throw new NotSupportedException("仅支持RSA私钥");
                }
            }
            catch (Exception ex)
            {
                _logger.LogError(ex, "使用BouncyCastle加载证书失败");
                throw;
            }
        }

        public async Task<string> SendGetRequestAsync(string path)
        {
            var url = _baseUrl + path.TrimStart('/');
            _logger.LogInformation("发送GET请求到: {Url}", url);

            try
            {
                var response = await _httpClient.GetAsync(url);
                _logger.LogInformation("响应状态码: {StatusCode}", response.StatusCode);

                response.EnsureSuccessStatusCode();

                var responseBody = await response.Content.ReadAsStringAsync();
                _logger.LogInformation("响应内容长度: {Length} 字符", responseBody.Length);

                return responseBody;
            }
            catch (HttpRequestException ex)
            {
                _logger.LogError(ex, "HTTP请求失败: {Url}", url);
                throw;
            }
        }

        public async Task<string> SendPostRequestAsync(string path, string jsonBody)
        {
            var url = _baseUrl + path.TrimStart('/');
            _logger.LogInformation("发送POST请求到: {Url}", url);

            try
            {
                var content = new StringContent(jsonBody, Encoding.UTF8, "application/json");
                var response = await _httpClient.PostAsync(url, content);
                _logger.LogInformation("响应状态码: {StatusCode}", response.StatusCode);

                response.EnsureSuccessStatusCode();

                var responseBody = await response.Content.ReadAsStringAsync();
                _logger.LogInformation("响应内容长度: {Length} 字符", responseBody.Length);

                return responseBody;
            }
            catch (HttpRequestException ex)
            {
                _logger.LogError(ex, "HTTP请求失败: {Url}", url);
                throw;
            }
        }

        public void Dispose()
        {
            _httpClient?.Dispose();
            _httpClientHandler?.Dispose();
        }
    }
}