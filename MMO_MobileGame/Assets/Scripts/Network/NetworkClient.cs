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

    private void Start()
    {
        if (kingdomUI)
            kingdomUI.Hide();
        
        if (resourceUI)
            resourceUI.Hide();
    }

    public void ConnectAndLogin(string username, string password)
    {
        if (!Library.Initialize())
        {
            Debug.LogError("Erreur d'initialisation ENet.");
            return;
        }

        if (client != null)
            client.Dispose();
        
        client = new Host();
        Address address = new Address();
        address.SetHost(serverIP);
        address.Port = serverPort;

        client.Create();

        _currentUsername = username;
        _currentPassword = password;

        Debug.Log($"Connexion au serveur {serverIP}:{serverPort}...");
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
        Debug.Log("<color=green>Connecte au serveur ! Envoi du Login...</color>");
        SendLogin(_currentUsername, _currentPassword);
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

            default:
                break;
        }
    }

    private void HandleLoginResult(byte[] payload)
    {
        LoginResult result = LoginResult.GetRootAsLoginResult(new ByteBuffer(payload));
        if (loginUI)
            loginUI.SetLoginResult(result.Success, result.Message);

        if (result.Success)
        {
            _accountId = result.AccountId;
            Debug.Log($"<color=cyan>Login OK. Demande liste royaumes...</color>");
            
            // Demander la liste des royaumes (meme connexion)
            var builder = new FlatBufferBuilder(16);
            RequestKingdoms.StartRequestKingdoms(builder);
            
            var req = RequestKingdoms.EndRequestKingdoms(builder);
            builder.Finish(req.Value);
            
            SendEnvelope(Opcode.C2S_RequestKingdoms, builder.SizedByteArray());
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

    private void SendPing()
    {
        var builder = new FlatBufferBuilder(16);
        MMO.Network.Ping.StartPing(builder);
        MMO.Network.Ping.AddTimestamp(builder, System.DateTimeOffset.UtcNow.ToUnixTimeMilliseconds());
        builder.Finish(MMO.Network.Ping.EndPing(builder).Value);
        SendEnvelope(Opcode.C2S_Ping, builder.SizedByteArray());
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
