using System;
using UnityEngine;
using ENet;
using Google.FlatBuffers;
using MMO.Network;


public class NetworkClient : MonoBehaviour
{
    [Header("Configuration")]
    public string serverIP = "127.0.0.1";
    public ushort port = 7777;

    private Host client;
    private Peer peer;
    
    // On garde bool pour savoir si on est connecte
    private bool isConnected = false;

    private void Start()
    {
        // 1. Initialiser la librairie ENet
        if (!Library.Initialize())
        {
            Debug.LogError("Erreur lors de l'initialisation de ENet.");
            return;
        }

        // 2. Creer l'hote client
        client = new Host();
        Address address = new Address();
        address.SetHost(serverIP);
        address.Port = port;

        // Le client n'a besoin d'ecouter sur aucun port en particulier (on met 0)
        client.Create();

        // 3. Lancer la connexion (doit correspondre au host C++ qui accepte 2 channels par defaut)
        Debug.Log($"Tentative de connexion au serveur {serverIP}:{port}...");
        peer = client.Connect(address, 2); // 2 channels de communication max (comme config.maxPlayers, 2, 0, 0 en C++)
    }

    private void Update()
    {
        if (client == null) return;

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
                    Debug.Log("<color=green>Client C# : Connexion reussie au serveur C++ !</color>");
                    isConnected = true;
                    SendPing();
                    break;

                case ENet.EventType.Receive:
                    Debug.Log($"Paquet recu ! (Taille: {netEvent.Packet.Length})");
                    netEvent.Packet.Dispose();
                    break;

                case ENet.EventType.Timeout:
                    Debug.LogWarning("Time-out de la connexion.");
                    isConnected = false;
                    break;

                case ENet.EventType.Disconnect:
                    Debug.Log("<color=red>Client C# : Deconnecte du serveur.</color>");
                    isConnected = false;
                    break;
            }
        }
    }

    private void SendPing()
    {
        // 1. Serialization du Payload Ping
        var pingBuilder = new FlatBufferBuilder(64);
        long currentTimestamp = DateTimeOffset.UtcNow.ToUnixTimeMilliseconds();
        MMO.Network.Ping.StartPing(pingBuilder);
        MMO.Network.Ping.AddTimestamp(pingBuilder, currentTimestamp);
        var pingOffset = MMO.Network.Ping.EndPing(pingBuilder);
        pingBuilder.Finish(pingOffset.Value);
        byte[] pingBytes = pingBuilder.SizedByteArray();

        // 2. Encapsulation dans l'Enveloppe (O(1) Dispatch)
        var envBuilder = new FlatBufferBuilder(128);
        var payloadVector = Envelope.CreatePayloadDataVector(envBuilder, pingBytes);
        var envOffset = Envelope.CreateEnvelope(envBuilder, Opcode.C2S_Ping, payloadVector);
        envBuilder.Finish(envOffset.Value);
        byte[] finalBytes = envBuilder.SizedByteArray();

        // 3. Transmission et Flush immediat
        ENet.Packet packet = default(ENet.Packet);
        packet.Create(finalBytes, ENet.PacketFlags.Reliable);
        peer.Send(0, ref packet);
        client.Flush();
        
        Debug.Log($"Ping {currentTimestamp} envoye avec succes en FlatBuffers au serveur !");
    }

    private void OnDestroy()
    {
        // 5. Nettoyage et deconnexion propre lors de la fermeture de Unity
        if (client != null)
        {
            if (peer.IsSet) 
            {
                peer.Disconnect(0);
                
                // Petit hack pour forcer l'envoi du paquet de deconnexion avant que Unity ferme brutalement
                client.Flush();
            }
            
            client.Dispose();
        }
        Library.Deinitialize();
    }
}
