using UnityEngine;
using UnityEngine.UI;
using TMPro;

public class LoginUI : MonoBehaviour
{
    [Header("Network References")]
    public NetworkClient networkClient;

    [Header("UI Elements")]
    public TMP_InputField usernameInput;
    public TMP_InputField passwordInput;
    public Button loginButton;
    public TextMeshProUGUI statusText;

    private void Start()
    {
        // On s'assure que le mot de passe s'affiche avec des etoiles
        if (passwordInput != null)
        {
            passwordInput.contentType = TMP_InputField.ContentType.Password;
        }

        if (loginButton != null)
        {
            loginButton.onClick.AddListener(OnLoginClicked);
        }

        if (statusText != null)
        {
            statusText.text = "Pret a vous connecter.";
            statusText.color = Color.white;
        }
    }

    private void OnLoginClicked()
    {
        string user = usernameInput.text.Trim();
        string pass = passwordInput.text.Trim();

        if (string.IsNullOrEmpty(user) || string.IsNullOrEmpty(pass))
        {
            SetStatus("L'identifiant et le mot de passe sont requis.", Color.red);
            return;
        }

        if (networkClient == null)
        {
            SetStatus("NetworkClient non assigne dans l'inspecteur.", Color.red);
            return;
        }

        SetStatus("Connexion en cours...", Color.yellow);
        
        loginButton.interactable = false;
        networkClient.ConnectAndLogin(user, pass);
    }

    public void SetLoginResult(bool success, string message)
    {
        loginButton.interactable = true;
        
        if (success)
        {
            SetStatus(message, Color.green);
            gameObject.SetActive(false);
        }
        else
        {
            SetStatus(message, Color.red);
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
