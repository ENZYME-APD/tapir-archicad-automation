using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Net.Http;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace TapirGrasshopperPlugin.Utilities
{
    public static class ConnectionSettings
    {
        public static int Port
        {
            get { return port; }
            set { port = value; }
        }

        private static int port = 19723;
    }

    public class CommandError
    {
        [JsonProperty ("code")]
        public int Code { get; set; }

        [JsonProperty ("message")]
        public string Message { get; set; }
    }

    public class CommandResponse
    {
        public string GetErrorMessage ()
        {
            if (Error == null || Error.Message == null) {
                return "Unknown Error";
            }
            return Error.Message;
        }

        [JsonProperty ("succeeded")]
        public bool Succeeded { get; set; }

        [JsonProperty ("error")]
        public CommandError Error { get; set; }

        [JsonProperty ("result")]
        public JObject Result { get; set; }
    }

    public class ArchicadConnection
    {
        public ArchicadConnection (int port)
        {
            mPort = port;
        }

        public CommandResponse SendCommand (string commandName, JObject commandParameters)
        {
            var task = Task.Run (() =>
                SendCommandAsync (commandName, commandParameters)
            );
            task.Wait ();
            return task.Result;
        }

        public CommandResponse SendAddOnCommand (string commandNamespace, string commandName, JObject commandParameters)
        {
            var task = Task.Run (() =>
                SendAddOnCommandAsync (commandNamespace, commandName, commandParameters)
            );
            task.Wait ();
            return task.Result;
        }

        private async Task<CommandResponse> SendCommandAsync (string commandName, JObject commandParameters)
        {
            try {
                JObject commandObject = JObject.FromObject (new {
                    command = "API." + commandName,
                    parameters = commandParameters
                });

                HttpClient client = new HttpClient ();
                client.BaseAddress = new Uri (string.Format ("http://127.0.0.1:{0}", mPort));

                HttpContent requestContent = new StringContent (commandObject.ToString (), Encoding.UTF8, "application/json");
                HttpResponseMessage response = await client.PostAsync ("", requestContent);
                var responseString = await response.Content.ReadAsStringAsync ();

                return JsonConvert.DeserializeObject<CommandResponse> (responseString);
            } catch (Exception) {
                return new CommandResponse () {
                    Succeeded = false,
                    Error = new CommandError () {
                        Code = 100,
                        Message = "Failed to connect to Archicad."
                    },
                    Result = null
                };
            }
        }

        private async Task<CommandResponse> SendAddOnCommandAsync (string commandNamespace, string commandName, JObject commandParameters)
        {
            JObject parametersObject = JObject.FromObject (new {
                addOnCommandId = new {
                    commandNamespace,
                    commandName
                },
                addOnCommandParameters = commandParameters
            });
            var result = await SendCommandAsync ("ExecuteAddOnCommand", parametersObject);
            if (result.Succeeded && result.Result != null) {
                result.Result = (JObject) result.Result["addOnCommandResponse"];
            }
            return result;
        }

        private int mPort;
    }
}
