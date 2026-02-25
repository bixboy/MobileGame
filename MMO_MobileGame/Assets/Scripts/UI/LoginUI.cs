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
    public Button guestButton;
    public Button googleLoginButton;
    public FirebaseManager firebaseManager;
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

        if (guestButton != null)
        {
            guestButton.onClick.AddListener(OnGuestClicked);
        }

        if (googleLoginButton != null)
        {
            googleLoginButton.onClick.AddListener(OnGoogleLoginClicked);
        }

        if (statusText != null)
        {
            statusText.text = "Pret a vous connecter.";
            statusText.color = Color.white;
        }

        // Tente de se reconnecter automatiquement
        if (networkClient != null)
        {
            networkClient.TryAutoReconnect();
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

    private void OnGuestClicked()
    {
        if (networkClient == null)
        {
            SetStatus("NetworkClient non assigne dans l'inspecteur.", Color.red);
            return;
        }

        SetStatus("Connexion invite en cours...", Color.yellow);
        
        if (loginButton) loginButton.interactable = false;
        if (guestButton) guestButton.interactable = false;
        
        networkClient.ConnectAsGuest();
    }

    private void OnGoogleLoginClicked()
    {
        if (firebaseManager == null)
        {
            SetStatus("FirebaseManager non assigne !", Color.red);
            return;
        }

        SetStatus("Authentification Google...", Color.yellow);
        firebaseManager.SignInWithGoogle();
    }

    public void SetLoginResult(bool success, string message)
    {
        if (loginButton) loginButton.interactable = true;
        if (guestButton) guestButton.interactable = true;
        
        if (success)
        {
            SetStatus(message, Color.green);
            // On ne fait plus gameObject.SetActive(false) ici car si LoginUI
            // est attache au meme GameObject que le Menu/Canvas, ca desactive tout !
            // Laisse NetworkClient appeler des methodes Hide() specifiques si besoin, ou on cache les boutons.
            if (loginButton) loginButton.gameObject.SetActive(false);
            if (guestButton) guestButton.gameObject.SetActive(false);
            if (googleLoginButton) googleLoginButton.gameObject.SetActive(false);
            if (usernameInput) usernameInput.gameObject.SetActive(false);
            if (passwordInput) passwordInput.gameObject.SetActive(false);
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
