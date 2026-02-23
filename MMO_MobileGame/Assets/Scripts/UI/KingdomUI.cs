using System.Collections.Generic;
using UnityEngine;
using UnityEngine.UI;
using TMPro;

public class KingdomUI : MonoBehaviour
{
    public struct KingdomData
    {
        public int id;
        public string name;
        public int players;
        public int maxPlayers;
    }

    [Header("Network References")]
    public NetworkClient networkClient;

    [Header("UI Elements")]
    public GameObject kingdomPanel;
    public Transform listContainer;
    public GameObject kingdomButtonPrefab;
    public TextMeshProUGUI statusText;

    private void Start()
    {
        // Masque au debut
        Hide();
    }

    public void Show()
    {
        if (kingdomPanel)
        {
            kingdomPanel.SetActive(true);
        }
        SetStatus("Selectionnez un royaume...", Color.white);
    }

    public void Hide()
    {
        if (kingdomPanel)
        {
            kingdomPanel.SetActive(false);
        }
    }

    public void ShowKingdoms(List<KingdomData> kingdoms)
    {
        Show();

        // Nettoie l'ancien contenu
        if (listContainer)
        {
            foreach (Transform child in listContainer)
            {
                Destroy(child.gameObject);
            }

            // Popule la nouvelle liste
            foreach (var k in kingdoms)
            {
                if (kingdomButtonPrefab)
                {
                    GameObject btnObj = Instantiate(kingdomButtonPrefab, listContainer);
                    
                    TextMeshProUGUI[] texts = btnObj.GetComponentsInChildren<TextMeshProUGUI>();
                    if (texts.Length > 0)
                        texts[0].text = k.name;
                    
                    if (texts.Length > 1)
                        texts[1].text = $"{k.players}/{k.maxPlayers}";

                    Button btn = btnObj.GetComponent<Button>();
                    if (btn)
                    {
                        btn.onClick.AddListener(() => OnKingdomClicked(k.id, k.name));
                    }
                }
            }
        }
        else
        {
            Debug.LogWarning("KingdomUI: 'listContainer' non assigne ! Impossible de creer les boutons.");
        }
    }

    private void OnKingdomClicked(int kingdomId, string kingdomName)
    {
        if (networkClient)
        {
            SetStatus($"Connexion a {kingdomName}...", Color.yellow);
            networkClient.SendSelectKingdom(kingdomId);
        }
    }

    private void SetStatus(string message, Color color)
    {
        if (statusText)
        {
            statusText.text = message;
            statusText.color = color;
        }
    }
}
