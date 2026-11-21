using Newtonsoft.Json.Linq;
using System;
using System.IO;
using System.Net;
using System.Net.Http;
using System.Net.Sockets;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using TapirGrasshopperPlugin.Types.ArchiCad;
using Xunit;

namespace TapirGrasshopperPlugin.Tests
{
    public class ArchicadConnectionTests
    {
        [Fact]
        public void TheClientIsCreatedOnce()
        {
            var first = ArchicadConnection.Client;
            var second = ArchicadConnection.Client;

            Assert.NotNull(first);
            Assert.Same(first, second);
            Assert.Null(first.BaseAddress);
        }

        [Fact]
        public void TheTimeoutIsInfinite()
        {
            Assert.Equal(
                Timeout.InfiniteTimeSpan,
                ArchicadConnection.Client.Timeout);
        }

        [Fact]
        public void ARefusedConnectionReturnsCode100()
        {
            var connection = new ArchicadConnection(GetClosedPort());

            var response = connection.SendCommand(
                "IsAlive",
                new JObject());

            Assert.False(response.Succeeded);
            Assert.Equal(100, response.Error.Code);
            Assert.Equal("Failed to connect to Archicad.", response.Error.Message);
        }

        [Fact]
        public void AnInvalidAnswerReturnsADifferentMessage()
        {
            using (var server = new OneAnswerServer("this is not JSON"))
            {
                var connection = new ArchicadConnection(server.Port);

                var response = connection.SendCommand(
                    "IsAlive",
                    new JObject());

                Assert.False(response.Succeeded);
                Assert.NotEqual(100, response.Error.Code);
                Assert.StartsWith(
                    ArchicadConnection.CommunicationFailedErrorMessage,
                    response.Error.Message);
                Assert.NotEqual("Failed to connect to Archicad.", response.Error.Message);
            }
        }

        [Fact]
        public void TheSharedClientFollowsThePortOfEachConnection()
        {
            using (var first = new OneAnswerServer(
                       "{\"succeeded\":true,\"result\":{\"port\":1}}"))
            using (var second = new OneAnswerServer(
                       "{\"succeeded\":true,\"result\":{\"port\":2}}"))
            {
                var firstResponse = new ArchicadConnection(first.Port)
                    .SendCommand("IsAlive", new JObject());
                var secondResponse = new ArchicadConnection(second.Port)
                    .SendCommand("IsAlive", new JObject());

                Assert.True(firstResponse.Succeeded);
                Assert.Equal(1, (int)firstResponse.Result["port"]);
                Assert.True(secondResponse.Succeeded);
                Assert.Equal(2, (int)secondResponse.Result["port"]);
            }
        }

        [Fact]
        public void ConnectFailuresAreConnectionFailures()
        {
            Assert.True(ArchicadConnection.IsConnectionFailure(
                new HttpRequestException(
                    "An error occurred while sending the request.",
                    new SocketException((int)SocketError.ConnectionRefused))));

            // .NET Framework (Rhino 7) wraps it in a WebException.
            Assert.True(ArchicadConnection.IsConnectionFailure(
                new HttpRequestException(
                    "An error occurred while sending the request.",
                    new WebException(
                        "Unable to connect to the remote server",
                        WebExceptionStatus.ConnectFailure))));

            Assert.True(ArchicadConnection.IsConnectionFailure(
                new AggregateException(
                    new SocketException((int)SocketError.ConnectionRefused))));
        }

        [Fact]
        public void OtherExceptionsAreNotConnectionFailures()
        {
            Exception[] others =
            {
                new TaskCanceledException("The request was canceled."),
                new HttpRequestException(
                    "An error occurred while sending the request.",
                    new IOException("The response ended prematurely.")),
                new HttpRequestException(
                    "An error occurred while sending the request.",
                    new SocketException((int)SocketError.ConnectionReset)),
                new Newtonsoft.Json.JsonReaderException("Unexpected character."),
                new InvalidOperationException("Something else.")
            };

            foreach (var other in others)
            {
                Assert.False(ArchicadConnection.IsConnectionFailure(other));

                var response = ArchicadConnection.CreateErrorResponse(other);
                Assert.False(response.Succeeded);
                Assert.Equal(101, response.Error.Code);
                Assert.Equal(
                    ArchicadConnection.CommunicationFailedErrorMessage +
                    other.GetBaseException().Message,
                    response.Error.Message);
            }
        }

        private static int GetClosedPort()
        {
            var listener = new TcpListener(IPAddress.Loopback, 0);
            listener.Start();
            var port = ((IPEndPoint)listener.LocalEndpoint).Port;
            listener.Stop();
            return port;
        }

        // Answers one HTTP request on a free loopback port with the given body.
        private sealed class OneAnswerServer : IDisposable
        {
            public OneAnswerServer(
                string body)
            {
                mListener = new TcpListener(IPAddress.Loopback, 0);
                mListener.Start();
                Port = ((IPEndPoint)mListener.LocalEndpoint).Port;
                mServing = Task.Run(() => Serve(body));
            }

            public int Port { get; }

            public void Dispose()
            {
                mListener.Stop();
                try
                {
                    mServing.Wait(TimeSpan.FromSeconds(5));
                }
                catch (AggregateException)
                {
                }
            }

            private async Task Serve(
                string body)
            {
                using (var client = await mListener.AcceptTcpClientAsync())
                using (var stream = client.GetStream())
                {
                    await ReadRequest(stream);

                    var bodyBytes = Encoding.UTF8.GetBytes(body);
                    var header = Encoding.ASCII.GetBytes(
                        "HTTP/1.1 200 OK\r\n" +
                        "Content-Type: application/json\r\n" +
                        "Content-Length: " + bodyBytes.Length + "\r\n" +
                        "Connection: close\r\n\r\n");
                    await stream.WriteAsync(header, 0, header.Length);
                    await stream.WriteAsync(bodyBytes, 0, bodyBytes.Length);
                    await stream.FlushAsync();
                }
            }

            private static async Task ReadRequest(
                NetworkStream stream)
            {
                var received = new MemoryStream();
                var buffer = new byte[4096];
                var contentLength = -1;
                var headerEnd = -1;
                while (headerEnd < 0 || received.Length < headerEnd + contentLength)
                {
                    var read = await stream.ReadAsync(buffer, 0, buffer.Length);
                    if (read == 0)
                    {
                        return;
                    }

                    received.Write(buffer, 0, read);
                    if (headerEnd < 0)
                    {
                        var text = Encoding.ASCII.GetString(received.ToArray());
                        var end = text.IndexOf("\r\n\r\n", StringComparison.Ordinal);
                        if (end >= 0)
                        {
                            headerEnd = end + 4;
                            contentLength = 0;
                            foreach (var line in text.Substring(0, end).Split("\r\n"))
                            {
                                if (line.StartsWith("Content-Length:", StringComparison.OrdinalIgnoreCase))
                                {
                                    contentLength = int.Parse(line.Substring(15).Trim());
                                }
                            }
                        }
                    }
                }
            }

            private readonly TcpListener mListener;
            private readonly Task mServing;
        }
    }
}
