using System;
using System.Runtime.InteropServices;
using UnityEngine;
using ENet;

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

        // 3. Lancer la connexion
        Debug.Log($"Tentative de connexion au serveur {serverIP}:{port}...");
        peer = client.Connect(address, 4); // 4 channels de communication max
    }

    private void Update()
    {
        if (client == null) return;

        // Variable pour stocker l'evenement recu
        ENet.Event netEvent;

        // 4. Depiler les messages recus du serveur (Non-bloquant)
        while (client.Service(0, out netEvent) > 0)
        {
            switch (netEvent.Type)
            {
                case ENet.EventType.Connect:
                    Debug.Log("<color=green>Client C# : Connexion reussie au serveur C++ !</color>");
                    isConnected = true;
                    
                    // --- TEST: On envoie un ping immediatement apres la connexion ---
                    SendPing();
                    break;

                case ENet.EventType.Receive:
                    Debug.Log($"Paquet recu ! (Taille: {netEvent.Packet.Length})");
                    // On detruit toujours le paquet ENet apres l'avoir lu (pour eviter les fuites memoire)
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
        // TODO: (Prochaine etape, integration de Flatbuffers C#)
        Debug.Log("TODO: Envoyer le paquet Ping en FlatBuffers.");
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
