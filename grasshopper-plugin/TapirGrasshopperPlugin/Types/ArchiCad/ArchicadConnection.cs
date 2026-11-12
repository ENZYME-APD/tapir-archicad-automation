using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using System;
using System.Linq;
using System.Net;
using System.Net.Http;
using System.Net.Sockets;
using System.Text;
using System.Threading;
using System.Threading.Tasks;

namespace TapirGrasshopperPlugin.Types.ArchiCad
{
    public static class ConnectionSettings
    {
        public static int Port
        {
            get => port;
            set => port = value;
        }

        private static int port = 19723;
    }

    public class CommandError
    {
        [JsonProperty("code")]
        public int Code { get; set; }

        [JsonProperty("message")]
        public string Message { get; set; }
    }

    public class CommandResponse
    {
        public string GetErrorMessage()
        {
            if (Error == null || Error.Message == null)
            {
                return "Unknown Error";
            }

            return Error.Message;
        }

        [JsonProperty("succeeded")]
        public bool Succeeded { get; set; }

        [JsonProperty("error")]
        public CommandError Error { get; set; }

        [JsonProperty("result")]
        public JObject Result { get; set; }
    }

    public class ArchicadConnection
    {
        public ArchicadConnection(
            int port)
        {
            mPort = port;
        }

        public CommandResponse SendCommand(
            string commandName,
            JObject commandParameters)
        {
            var task = Task.Run(() => SendCommandAsync(
                commandName,
                commandParameters));
            task.Wait();
            return task.Result;
        }

        public CommandResponse SendAddOnCommand(
            string commandNamespace,
            string commandName,
            JObject commandParameters)
        {
            RemoveNullProperties(commandParameters);

            var task = Task.Run(() => SendAddOnCommandAsync(
                commandNamespace,
                commandName,
                commandParameters));
            task.Wait();
            return task.Result;
        }

        private async Task<CommandResponse> SendCommandAsync(
            string commandName,
            JObject commandParameters)
        {
            try
            {
                var commandObject = JObject.FromObject(
                    new
                    {
                        command = "API." + commandName,
                        parameters = commandParameters
                    });

                // The port can change between calls (ConnectionSettings.Port),
                // so the address is built per request, not set as BaseAddress
                // of the shared client.
                var requestUri = new Uri(
                    string.Format(
                        "http://127.0.0.1:{0}/",
                        mPort));

                using (HttpContent requestContent = new StringContent(
                           commandObject.ToString(),
                           Encoding.UTF8,
                           "application/json"))
                using (var response = await Client.PostAsync(
                           requestUri,
                           requestContent))
                {
                    var responseString =
                        await response.Content.ReadAsStringAsync();

                    return JsonConvert.DeserializeObject<CommandResponse>(
                        responseString);
                }
            }
            catch (Exception e)
            {
                return CreateErrorResponse(e);
            }
        }

        public const int ConnectionFailedErrorCode = 100;
        public const string ConnectionFailedErrorMessage =
            "Failed to connect to Archicad.";
        public const int CommunicationFailedErrorCode = 101;
        public const string CommunicationFailedErrorMessage =
            "Archicad was reached, but the command failed while communicating with it: ";

        // One client for every command: a new HttpClient per call opens a new
        // connection that is only released by the garbage collector. Commands
        // wait as long as Archicad takes (some run for minutes on large
        // projects), as aclib does, instead of HttpClient's default 100 s.
        internal static readonly HttpClient Client = new HttpClient()
        {
            Timeout = Timeout.InfiniteTimeSpan
        };

        // Archicad not running or not listening on the port keeps code 100 and
        // its message, which saved definitions may rely on; any other failure
        // (the connection dropped, an answer that is not valid JSON, ...) gets
        // its own code and says what went wrong.
        internal static CommandResponse CreateErrorResponse(
            Exception e)
        {
            if (IsConnectionFailure(e))
            {
                return new CommandResponse()
                {
                    Succeeded = false,
                    Error = new CommandError()
                    {
                        Code = ConnectionFailedErrorCode,
                        Message = ConnectionFailedErrorMessage
                    },
                    Result = null
                };
            }

            return new CommandResponse()
            {
                Succeeded = false,
                Error = new CommandError()
                {
                    Code = CommunicationFailedErrorCode,
                    Message = CommunicationFailedErrorMessage +
                              e.GetBaseException().Message
                },
                Result = null
            };
        }

        internal static bool IsConnectionFailure(
            Exception e)
        {
            for (var current = e; current != null; current = current.InnerException)
            {
                if (current is AggregateException aggregate &&
                    aggregate.InnerExceptions.Any(IsConnectionFailure))
                {
                    return true;
                }

                if (current is SocketException socketException)
                {
                    switch (socketException.SocketErrorCode)
                    {
                        case SocketError.ConnectionRefused:
                        case SocketError.HostUnreachable:
                        case SocketError.NetworkUnreachable:
                        case SocketError.HostNotFound:
                        case SocketError.AddressNotAvailable:
                            return true;
                    }
                }

                // .NET Framework's HttpClient reports a failed connect as a
                // WebException.
                if (current is WebException webException &&
                    (webException.Status == WebExceptionStatus.ConnectFailure ||
                     webException.Status ==
                     WebExceptionStatus.NameResolutionFailure))
                {
                    return true;
                }
            }

            return false;
        }

        private async Task<CommandResponse> SendAddOnCommandAsync(
            string commandNamespace,
            string commandName,
            JObject commandParameters)
        {
            var parametersObject = JObject.FromObject(
                new
                {
                    addOnCommandId = new { commandNamespace, commandName },
                    addOnCommandParameters = commandParameters
                });
            var result = await SendCommandAsync(
                "ExecuteAddOnCommand",
                parametersObject);
            if (result.Succeeded && result.Result != null)
            {
                result.Result = (JObject)result.Result["addOnCommandResponse"];
            }

            return result;
        }

        // Archicad validates the parameters against the command's input schema,
        // and no Tapir schema accepts null: an optional field the component
        // leaves unset has to be left out, not sent as null. Serializing the
        // parameter objects writes their unset fields as null, so they are
        // dropped here, at every depth, before the command is sent.
        public static void RemoveNullProperties(
            JToken token)
        {
            if (token is JObject obj)
            {
                foreach (var property in obj.Properties().ToList())
                {
                    if (property.Value.Type == JTokenType.Null)
                    {
                        property.Remove();
                    }
                    else
                    {
                        RemoveNullProperties(property.Value);
                    }
                }
            }
            else if (token is JArray array)
            {
                foreach (var item in array)
                {
                    RemoveNullProperties(item);
                }
            }
        }

        private int mPort;
    }
}