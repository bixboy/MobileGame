using System;
using System.Collections.Generic;
using UnityEngine;
using ENet;
using Google.FlatBuffers;
using MMO.Network;

public enum ClientConnectionState
{
    Disconnected,
    Connected,
    InKingdom
}

public class NetworkClient : MonoBehaviour
{
    [Header("UI")]
    public LoginUI loginUI;
    public ResourceUI resourceUI;
    public KingdomUI kingdomUI;

    [Header("Configuration")]
    public string serverIP = "127.0.0.1";
    public ushort serverPort = 7777;

    private Host client;
    private Peer peer;
    
    public ClientConnectionState ConnectionState { get; private set; } = ClientConnectionState.Disconnected;
    
    private string _currentUsername;
    private string _currentPassword;
    private int _accountId;

    public bool IsConnectedAsGuest()
    {
        // Pour l'instant, on considère que si on est connecté (InKingdom ou Connected)
        // et qu'on a un accountId mais qu'on a utilisé le flow Guest, c'est un invité.
        return ConnectionState != ClientConnectionState.Disconnected && _currentAuthMode == AuthMode.Guest;
    }
    
    // Auth Method Tracking
    private enum AuthMode { None, Classic, Guest, Reconnect, Social }
    private AuthMode _currentAuthMode = AuthMode.None;

    private string _socialProvider;
    private string _socialId;
    private string _socialIdToken;

    private void Start()
    {
        if (kingdomUI)
            kingdomUI.Hide();
        
        if (resourceUI)
            resourceUI.Hide();
    }

    public void ConnectAndLogin(string username, string password)
    {
        _currentUsername = username;
        _currentPassword = password;
        _currentAuthMode = AuthMode.Classic;
        StartConnection();
    }

    public void ConnectAsGuest()
    {
        _currentAuthMode = AuthMode.Guest;
        StartConnection();
    }

    public void TryAutoReconnect()
    {
        if (PlayerPrefs.HasKey("SessionToken") && PlayerPrefs.HasKey("AccountId"))
        {
            _currentAuthMode = AuthMode.Reconnect;
            StartConnection();
        }
        else
        {
            // Fallback to normal login UI
            if (loginUI) loginUI.gameObject.SetActive(true);
        }
    }

    public void ConnectWithSocialLogin(string provider, string socialId, string idToken)
    {
        _socialProvider = provider;
        _socialId = socialId;
        _socialIdToken = idToken;
        _currentAuthMode = AuthMode.Social;
        StartConnection();
    }

    public void DisconnectAndClearSession()
    {
        // Supprime le token de session
        PlayerPrefs.DeleteKey("SessionToken");
        PlayerPrefs.DeleteKey("AccountId");
        PlayerPrefs.Save();

        // Deconnecte le pair ENet
        if (client != null && peer.IsSet)
        {
            peer.Disconnect(0);
            client.Flush();
        }

        // Cache les UI du jeu et reaffiche le login
        if (resourceUI) resourceUI.Hide();
        if (kingdomUI) kingdomUI.Hide();
        if (loginUI)
        {
            loginUI.gameObject.SetActive(true);
            loginUI.SetLoginResult(false, "Deconnecte avec succes.");
        }

        ConnectionState = ClientConnectionState.Disconnected;
        Debug.Log("<color=red>Deconnecte et session effacee.</color>");
    }

    private void StartConnection()
    {
        if (!Library.Initialize())
        {
            Debug.LogError("Erreur d'initialisation ENet.");
            return;
        }

        if (client != null)
        {
            if (peer.IsSet) peer.DisconnectNow(0);
            client.Flush();
            client.Dispose();
            client = null;
        }
        
        client = new Host();
        Address address = new Address();
        address.SetHost(serverIP);
        address.Port = serverPort;

        client.Create();

        Debug.Log($"Connexion au serveur {serverIP}:{serverPort} (Mode: {_currentAuthMode})...");
        peer = client.Connect(address, 2);
    }

    public void SendSelectKingdom(int kingdomId)
    {
        var builder = new FlatBufferBuilder(32);
        SelectKingdom.StartSelectKingdom(builder);
        SelectKingdom.AddKingdomId(builder, kingdomId);
        
        var reqOffset = SelectKingdom.EndSelectKingdom(builder);
        builder.Finish(reqOffset.Value);
        
        SendEnvelope(Opcode.C2S_SelectKingdom, builder.SizedByteArray());
    }

    private void Update()
    {
        if (client == null)
            return;

        ENet.Event netEvent;
        bool polled = false;

        while (!polled)
        {
            if (client.CheckEvents(out netEvent) <= 0)
            {
                if (client.Service(15, out netEvent) <= 0)
                    break;
                
                polled = true;
            }

            switch (netEvent.Type)
            {
                case ENet.EventType.Connect:
                    OnConnected();
                    break;

                case ENet.EventType.Receive:
                    HandleReceive(ref netEvent);
                    netEvent.Packet.Dispose();
                    break;
                
                case ENet.EventType.Disconnect:
                    OnDisconnected();
                    break;
                
                case ENet.EventType.Timeout:
                    break;
            }
        }
    }

    private void OnConnected()
    {
        peer.PingInterval(2000); 
        InvokeRepeating(nameof(SendPing), 1f, 5f);

        ConnectionState = ClientConnectionState.Connected;
        Debug.Log("<color=green>Connecte au serveur ! Envoi de la requete d'auth...</color>");

        switch (_currentAuthMode)
        {
            case AuthMode.Classic:
                SendLogin(_currentUsername, _currentPassword);
                break;
            case AuthMode.Guest:
                SendGuestLogin();
                break;
            case AuthMode.Reconnect:
                SendReconnect();
                break;
            case AuthMode.Social:
                SendSocialLogin(_socialProvider, _socialId, _socialIdToken);
                break;
        }
    }

    private void OnDisconnected()
    {
        CancelInvoke(nameof(SendPing));
        Debug.LogWarning($"Deconnecte du serveur (Etat: {ConnectionState})");
        
        ConnectionState = ClientConnectionState.Disconnected;
        if (loginUI)
            loginUI.gameObject.SetActive(true);
        
        if (kingdomUI)
            kingdomUI.Hide();
        
        if (resourceUI)
            resourceUI.Hide();
    }

    private void HandleReceive(ref ENet.Event netEvent)
    {
        byte[] buffer = new byte[netEvent.Packet.Length];
        netEvent.Packet.CopyTo(buffer);

        ByteBuffer bb = new ByteBuffer(buffer);
        Envelope envelope = Envelope.GetRootAsEnvelope(bb);

        switch (envelope.Opcode)
        {
            case Opcode.S2C_LoginResult:
                HandleLoginResult(envelope.GetPayloadDataArray());
                break;

            case Opcode.S2C_KingdomList:
                HandleKingdomList(envelope.GetPayloadDataArray());
                break;

            case Opcode.S2C_PlayerData:
                HandlePlayerData(envelope.GetPayloadDataArray());
                break;

            case Opcode.S2C_ResourceUpdate:
                HandleResourceUpdate(envelope.GetPayloadDataArray());
                break;

            case Opcode.S2C_Pong:
                HandlePong(envelope.GetPayloadDataArray());
                break;

            case Opcode.S2C_BindAccountResult:
                HandleBindAccountResult(envelope.GetPayloadDataArray());
                break;

            case Opcode.S2C_BindSocialAccountResult:
                HandleBindSocialAccountResult(envelope.GetPayloadDataArray());
                break;

            default:
                break;
        }
    }

    private void HandleLoginResult(byte[] payload)
    {
        LoginResult result = LoginResult.GetRootAsLoginResult(new ByteBuffer(payload));
        
        if (!result.Success)
        {
            Debug.LogError($"Login failed: {result.Message}");
            ConnectionState = ClientConnectionState.Disconnected;
            _accountId = -1;
            
            PlayerPrefs.DeleteKey("SessionToken");
            PlayerPrefs.DeleteKey("AccountId");
            
            if (loginUI) loginUI.SetLoginResult(false, result.Message);
        }
        else
        {
            _accountId = result.AccountId;
            string token = result.SessionToken;
            
            PlayerPrefs.SetString("SessionToken", token);
            PlayerPrefs.SetInt("AccountId", _accountId);
            PlayerPrefs.Save();

            if (loginUI) loginUI.SetLoginResult(true, result.Message);

            Debug.Log($"<color=cyan>Login OK (ID: {_accountId}). Demande liste royaumes...</color>");
            
            // Demander la liste des royaumes (meme connexion)
            var builder = new FlatBufferBuilder(16);
            RequestKingdoms.StartRequestKingdoms(builder);
            
            var req = RequestKingdoms.EndRequestKingdoms(builder);
            builder.Finish(req.Value);
            
            SendEnvelope(Opcode.C2S_RequestKingdoms, builder.SizedByteArray());
        }
    }

    private void HandleBindAccountResult(byte[] payload)
    {
        BindAccountResult result = BindAccountResult.GetRootAsBindAccountResult(new ByteBuffer(payload));
        
        if (result.Success)
        {
            Debug.Log($"<color=green>Liaison de compte reussie ! Message : {result.Message}</color>");
            // TODO: Mettre a jour l'UI correspondante (ex: cacher le panel Link Account)
        }
        else
        {
            Debug.LogError($"Echec de la liaison de compte : {result.Message}");
            // TODO: Afficher l'erreur dans l'UI
        }
    }

    private void HandleBindSocialAccountResult(byte[] payload)
    {
        BindSocialAccountResult result = BindSocialAccountResult.GetRootAsBindSocialAccountResult(new ByteBuffer(payload));
        
        if (result.Success)
        {
            Debug.Log($"<color=green>Liaison de compte Social reussie ! Message : {result.Message}</color>");
            // TODO: Mettre a jour l'UI correspondante (ex: fermer le popup de liaison)
        }
        else
        {
            Debug.LogError($"Echec de la liaison Social : {result.Message}");
            // TODO: Afficher l'erreur dans l'UI
        }
    }

    private void HandleKingdomList(byte[] payload)
    {
        KingdomList list = KingdomList.GetRootAsKingdomList(new ByteBuffer(payload));
        Debug.Log($"<color=yellow>Recu liste de {list.KingdomsLength} royaumes.</color>");

        if (kingdomUI)
        {
            List<KingdomUI.KingdomData> uiData = new List<KingdomUI.KingdomData>();
            for (int i = 0; i < list.KingdomsLength; i++)
            {
                var k = list.Kingdoms(i).Value;
                uiData.Add(new KingdomUI.KingdomData { id = k.Id, name = k.Name, players = k.PlayerCount, maxPlayers = k.MaxPlayers });
            }
            
            kingdomUI.ShowKingdoms(uiData);
        }
    }

    private void HandlePlayerData(byte[] payload)
    {
        PlayerData playerData = PlayerData.GetRootAsPlayerData(new ByteBuffer(payload));
        ConnectionState = ClientConnectionState.InKingdom;
        Debug.Log($"<color=yellow>Bienvenue en jeu {playerData.Username} !</color>");

        if (kingdomUI)
            kingdomUI.Hide();

        if (resourceUI)
        {
            resourceUI.SetResources(playerData.Food, playerData.Wood, playerData.Stone, playerData.Gold);
            resourceUI.Show();
        }
    }

    private void HandleResourceUpdate(byte[] payload)
    {
        ResourceUpdate resUpdate = ResourceUpdate.GetRootAsResourceUpdate(new ByteBuffer(payload));
        if (resourceUI)
            resourceUI.SetResources(resUpdate.Food, resUpdate.Wood, resUpdate.Stone, resUpdate.Gold);
    }

    private void HandlePong(byte[] payload)
    {
        Pong pong = Pong.GetRootAsPong(new ByteBuffer(payload));
        long now = DateTimeOffset.UtcNow.ToUnixTimeMilliseconds();
        long rtt = now - pong.ClientTimestamp;
        Debug.Log($"<color=gray>RTT: {rtt}ms</color>");
    }

    private void SendLogin(string username, string password)
    {
        var builder = new FlatBufferBuilder(64);
        var nameOffset = builder.CreateString(username);
        var passOffset = builder.CreateString(password);
        Login.StartLogin(builder);
        Login.AddUsername(builder, nameOffset);
        Login.AddPassword(builder, passOffset);
        builder.Finish(Login.EndLogin(builder).Value);
        SendEnvelope(Opcode.C2S_Login, builder.SizedByteArray());
    }

    private void SendGuestLogin()
    {
        string deviceId = SystemInfo.deviceUniqueIdentifier;
        if (string.IsNullOrEmpty(deviceId))
            deviceId = "UNKNOWN_DEVICE_" + UnityEngine.Random.Range(1000, 9999);

        var builder = new FlatBufferBuilder(64);
        var idOffset = builder.CreateString(deviceId);
        GuestLogin.StartGuestLogin(builder);
        GuestLogin.AddDeviceId(builder, idOffset);
        builder.Finish(GuestLogin.EndGuestLogin(builder).Value);
        SendEnvelope(Opcode.C2S_GuestLogin, builder.SizedByteArray());
    }

    private void SendReconnect()
    {
        int savedId = PlayerPrefs.GetInt("AccountId", -1);
        string savedToken = PlayerPrefs.GetString("SessionToken", "");

        if (savedId == -1 || string.IsNullOrEmpty(savedToken))
        {
            Debug.LogWarning("Impossible d'auto-reconnect : données manquantes.");
            if (loginUI) loginUI.gameObject.SetActive(true);
            return;
        }

        var builder = new FlatBufferBuilder(64);
        var tokenOffset = builder.CreateString(savedToken);
        Reconnect.StartReconnect(builder);
        Reconnect.AddAccountId(builder, savedId);
        Reconnect.AddSessionToken(builder, tokenOffset);
        builder.Finish(Reconnect.EndReconnect(builder).Value);
        SendEnvelope(Opcode.C2S_Reconnect, builder.SizedByteArray());
    }

    private void SendPing()
    {
        var builder = new FlatBufferBuilder(16);
        MMO.Network.Ping.StartPing(builder);
        MMO.Network.Ping.AddTimestamp(builder, System.DateTimeOffset.UtcNow.ToUnixTimeMilliseconds());
        builder.Finish(MMO.Network.Ping.EndPing(builder).Value);
        SendEnvelope(Opcode.C2S_Ping, builder.SizedByteArray());
    }

    public void SendBindAccount(string username, string password)
    {
        if (ConnectionState == ClientConnectionState.Disconnected) return;

        var builder = new FlatBufferBuilder(64);
        var userOffset = builder.CreateString(username);
        var passOffset = builder.CreateString(password);
        
        BindAccount.StartBindAccount(builder);
        BindAccount.AddUsername(builder, userOffset);
        BindAccount.AddPassword(builder, passOffset);
        builder.Finish(BindAccount.EndBindAccount(builder).Value);
        
        SendEnvelope(Opcode.C2S_BindAccount, builder.SizedByteArray());
    }

    public void SendBindSocialAccount(string authProvider, string providerId, string idToken = "")
    {
        if (ConnectionState == ClientConnectionState.Disconnected) return;

        var builder = new FlatBufferBuilder(64);
        var providerOffset = builder.CreateString(authProvider);
        var providerIdOffset = builder.CreateString(providerId);
        var idTokenOffset = builder.CreateString(idToken);
        
        BindSocialAccount.StartBindSocialAccount(builder);
        BindSocialAccount.AddAuthProvider(builder, providerOffset);
        BindSocialAccount.AddProviderId(builder, providerIdOffset);
        BindSocialAccount.AddIdToken(builder, idTokenOffset);
        builder.Finish(BindSocialAccount.EndBindSocialAccount(builder).Value);
        
        SendEnvelope(Opcode.C2S_BindSocialAccount, builder.SizedByteArray());
    }

    public void SendSocialLogin(string authProvider, string providerId, string idToken = "")
    {
        if (ConnectionState == ClientConnectionState.Disconnected) return;

        var builder = new FlatBufferBuilder(64);
        var providerOffset = builder.CreateString(authProvider);
        var providerIdOffset = builder.CreateString(providerId);
        var idTokenOffset = builder.CreateString(idToken);
        
        SocialLogin.StartSocialLogin(builder);
        SocialLogin.AddAuthProvider(builder, providerOffset);
        SocialLogin.AddProviderId(builder, providerIdOffset);
        SocialLogin.AddIdToken(builder, idTokenOffset);
        builder.Finish(SocialLogin.EndSocialLogin(builder).Value);
        
        SendEnvelope(Opcode.C2S_SocialLogin, builder.SizedByteArray());
    }

    public void SendModifyResource(string resourceType, int delta)
    {
        if (ConnectionState != ClientConnectionState.InKingdom)
            return;

        ResourceType type;
        switch (resourceType.ToLower())
        {
            case "food":  
                type = ResourceType.Food;
                break;
            
            case "wood":
                type = ResourceType.Wood;
                break;
            
            case "stone":
                type = ResourceType.Stone;
                break;
            
            case "gold":
                type = ResourceType.Gold;
                break;
            
            default:
                Debug.LogWarning($"ResourceType inconnu: {resourceType}");
                return;
        }

        var builder = new FlatBufferBuilder(32);
        ModifyResources.StartModifyResources(builder);
        ModifyResources.AddResourceType(builder, type);
        ModifyResources.AddDelta(builder, delta);
        builder.Finish(ModifyResources.EndModifyResources(builder).Value);
        
        SendEnvelope(Opcode.C2S_ModifyResources, builder.SizedByteArray());
    }

    private void SendEnvelope(Opcode opcode, byte[] payloadBytes)
    {
        if (peer.State != ENet.PeerState.Connected)
            return;

        var envBuilder = new FlatBufferBuilder(128 + payloadBytes.Length);
        var payloadVector = Envelope.CreatePayloadDataVector(envBuilder, payloadBytes);
        var envOffset = Envelope.CreateEnvelope(envBuilder, opcode, payloadVector);
        envBuilder.Finish(envOffset.Value);
        
        byte[] finalBytes = envBuilder.SizedByteArray();
        Packet packet = default(Packet);
        packet.Create(finalBytes, PacketFlags.Reliable);
        
        peer.Send(0, ref packet);
        client.Flush();
    }

    private void OnDestroy()
    {
        if (client != null)
        {
            if (peer.IsSet) 
            {
                peer.Disconnect(0);
                client.Flush();
            }
            
            client.Dispose();
        }
        Library.Deinitialize();
    }
}
