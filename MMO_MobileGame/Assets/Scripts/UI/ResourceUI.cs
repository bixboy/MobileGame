using UnityEngine;
using UnityEngine.UI;
using TMPro;


public class ResourceUI : MonoBehaviour
{
    [Header("Network")]
    public NetworkClient networkClient;

    [Header("Food")]
    public TextMeshProUGUI foodText;
    public Button foodAddButton;
    public Button foodRemoveButton;

    [Header("Wood")]
    public TextMeshProUGUI woodText;
    public Button woodAddButton;
    public Button woodRemoveButton;

    [Header("Stone")]
    public TextMeshProUGUI stoneText;
    public Button stoneAddButton;
    public Button stoneRemoveButton;

    [Header("Gold")]
    public TextMeshProUGUI goldText;
    public Button goldAddButton;
    public Button goldRemoveButton;

    [Header("Configuration")]
    public int modifyAmount = 100;

    private void Start()
    {
        // Boutons Food
        if (foodAddButton)
            foodAddButton.onClick.AddListener(() => SendModify("food", modifyAmount));
        
        if (foodRemoveButton)
            foodRemoveButton.onClick.AddListener(() => SendModify("food", -modifyAmount));

        // Boutons Wood
        if (woodAddButton)
            woodAddButton.onClick.AddListener(() => SendModify("wood", modifyAmount));
        
        if (woodRemoveButton)
            woodRemoveButton.onClick.AddListener(() => SendModify("wood", -modifyAmount));

        // Boutons Stone
        if (stoneAddButton)
            stoneAddButton.onClick.AddListener(() => SendModify("stone", modifyAmount));
        
        if (stoneRemoveButton)
            stoneRemoveButton.onClick.AddListener(() => SendModify("stone", -modifyAmount));

        // Boutons Gold
        if (goldAddButton)
            goldAddButton.onClick.AddListener(() => SendModify("gold", modifyAmount));
        
        if (goldRemoveButton)
            goldRemoveButton.onClick.AddListener(() => SendModify("gold", -modifyAmount));

        gameObject.SetActive(false);
    }

    public void SetResources(int food, int wood, int stone, int gold)
    {
        if (foodText)
            foodText.text = food.ToString();
        
        if (woodText)
            woodText.text = wood.ToString();
        
        if (stoneText)
            stoneText.text = stone.ToString();
        
        if (goldText)
            goldText.text = gold.ToString();
    }

    public void Show()
    {
        gameObject.SetActive(true);
    }

    public void Hide()
    {
        gameObject.SetActive(false);
    }

    private void SendModify(string resourceType, int delta)
    {
        if (networkClient != null)
            networkClient.SendModifyResource(resourceType, delta);
    }
}
