using UnityEngine;
using UnityEngine.UI;
using TMPro;


public class ResourceUI : MonoBehaviour
{
    [Header("Network")]
    public NetworkClient networkClient;

    [Header("General")]
    public Button logoutButton;

    [Header("Account Binding")]
    public TMP_InputField bindUsernameInput;
    public TMP_InputField bindPasswordInput;
    public Button bindSubmitButton;
    public Button bindGoogleButton;
    public FirebaseManager firebaseManager;

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
            
        // Bouton deconnexion
        if (logoutButton)
            logoutButton.onClick.AddListener(() => { if (networkClient) networkClient.DisconnectAndClearSession(); });

        // Bouton de liaison de compte classique
        if (bindSubmitButton)
        {
            bindSubmitButton.onClick.AddListener(() =>
            {
                if (networkClient && bindUsernameInput && bindPasswordInput)
                {
                    string user = bindUsernameInput.text;
                    string pass = bindPasswordInput.text;
                    networkClient.SendBindAccount(user, pass);
                }
            });
        }

        if (bindGoogleButton != null)
        {
            bindGoogleButton.onClick.AddListener(() =>
            {
                if (firebaseManager != null)
                {
                    firebaseManager.RequestBind();
                }
            });
        }

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
