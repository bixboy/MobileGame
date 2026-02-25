using UnityEngine;
using Firebase;
using Firebase.Auth;
using Google;
using System.Collections;
using System.Threading.Tasks;
using TMPro;

public class FirebaseManager : MonoBehaviour
{
    [Header("Configuration")]
    public string webClientId = "494310374949-rqdbp4eepj4jaoorjimtia00faptrcd3.apps.googleusercontent.com";
    public string webClientSecret = "";
    public NetworkClient networkClient;
    public TextMeshProUGUI statusText;

    private FirebaseAuth auth;
    private FirebaseUser user;
    private OAuthDesktopFlow desktopFlow;

    private void Awake()
    {
        desktopFlow = GetComponent<OAuthDesktopFlow>();
        if (desktopFlow == null) desktopFlow = gameObject.AddComponent<OAuthDesktopFlow>();
        InitializeFirebase();
    }

    private void InitializeFirebase()
    {
        FirebaseApp.CheckAndFixDependenciesAsync().ContinueWith(task => {
            var dependencyStatus = task.Result;
            if (dependencyStatus == DependencyStatus.Available) {
                auth = FirebaseAuth.DefaultInstance;
                auth.StateChanged += AuthStateChanged;
                AuthStateChanged(this, null);
                Debug.Log("[Firebase] Initialized Successfully");
            } else {
                Debug.LogError($"[Firebase] Could not resolve dependencies: {dependencyStatus}");
            }
        });
    }

    void AuthStateChanged(object sender, System.EventArgs eventArgs)
    {
        if (auth.CurrentUser != user) {
            bool signedIn = user != auth.CurrentUser && auth.CurrentUser != null;
            if (!signedIn && user != null) {
                Debug.Log("[Firebase] Signed Out");
            }
            user = auth.CurrentUser;
            if (signedIn) {
                Debug.Log($"[Firebase] Signed In: {user.UserId}");
            }
        }
    }

    public void SignInWithGoogle()
    {
        if (auth == null) {
            SetStatus("Firebase non initialise.", Color.red);
            return;
        }

#if UNITY_EDITOR || UNITY_STANDALONE_WIN
        // Flow pour PC (Éditeur et Windows)
        SignInWithGooglePC();
#else
        // Flow pour Mobile (Android/iOS)
        SignInWithGoogleMobile();
#endif
    }

    private void SignInWithGoogleMobile()
    {
        GoogleSignInConfiguration config = new GoogleSignInConfiguration {
            WebClientId = webClientId,
            RequestIdToken = true,
            RequestEmail = true,
            UseGameSignIn = false
        };
        GoogleSignIn.Configuration = config;

        SetStatus("Google: Ouverture du selecteur...", Color.yellow);
        
        GoogleSignIn.DefaultInstance.SignIn().ContinueWith(task => {
            if (task.IsFaulted) {
                SetStatus("Google: Erreur de connexion.", Color.red);
                foreach (var exception in task.Exception.Flatten().InnerExceptions) {
                    Debug.LogError($@"[Google] {exception.Message}");
                }
            } else if (task.IsCanceled) {
                SetStatus("Google: Connexion annulee.", Color.yellow);
            } else {
                SetStatus("Firebase: Authentification...", Color.yellow);
                SignInWithFirebase(task.Result.IdToken);
            }
        });
    }

    private void SignInWithGooglePC()
    {
        SetStatus("PC: Ouverture du navigateur...", Color.yellow);
        
        // On récupère le IdToken via le flux loopback personnalisé
        desktopFlow.GetIdTokenAsync(webClientId, webClientSecret).ContinueWith(task => {
            if (task.IsFaulted || string.IsNullOrEmpty(task.Result)) {
                SetStatus("PC: Erreur ou annulation.", Color.red);
                if (task.Exception != null) Debug.LogError($@"[OAuth] {task.Exception.Flatten().Message}");
            } else {
                Debug.Log($@"[Firebase] OAuth Result: OK, starting Firebase sign-in...");
                SetStatus("Firebase: Authentification PC...", Color.yellow);
                SignInWithFirebase(task.Result);
            }
        });
    }

    private void SignInWithFirebase(string idToken)
    {
        Debug.Log("[Firebase] SignInWithFirebase starting...");
        Credential credential = GoogleAuthProvider.GetCredential(idToken, null);
        auth.SignInWithCredentialAsync(credential).ContinueWith(task => {
            if (task.IsFaulted || task.IsCanceled) {
                SetStatus("Firebase: Erreur d'auth.", Color.red);
                if (task.Exception != null) Debug.LogError($@"[Firebase] Auth Error: {task.Exception.Flatten().Message}");
                return;
            }

            FirebaseUser newUser = task.Result;
            Debug.Log($@"[Firebase] User signed in successfully: {newUser.DisplayName} ({newUser.UserId})");
            
            UnityMainThreadDispatcher.Instance().Enqueue(() => {
                // Une fois connecté à Firebase, on a le choix :
                // 1. Si on est déjà loggé en invité -> On BIND
                // 2. Si on n'est pas loggé -> On LOGIN social
                if (networkClient.IsConnectedAsGuest()) {
                    RequestBind(newUser);
                } else {
                    RequestSocialLogin(newUser);
                }
            });
        });
    }

    private void SetStatus(string msg, Color color)
    {
        UnityMainThreadDispatcher.Instance().Enqueue(() => {
            if (statusText) {
                statusText.text = msg;
                statusText.color = color;
            }
        });
    }

    public void RequestBind(FirebaseUser targetUser = null)
    {
        FirebaseUser fUser = targetUser ?? user;
        if (fUser == null) {
            if (statusText) statusText.text = "Firebase: Pas connecte.";
            return;
        }

        // On envoie le Uid Firebase et on récupère le token ID pour validation future
        fUser.TokenAsync(true).ContinueWith(task => {
            if (task.IsCompletedSuccessfully) {
                string idToken = task.Result;
                UnityMainThreadDispatcher.Instance().Enqueue(() => {
                    networkClient.SendBindSocialAccount("firebase", fUser.UserId, idToken);
                    if (statusText) statusText.text = "Firebase: Liaison en cours...";
                });
            }
        });
    }

    public void RequestSocialLogin(FirebaseUser targetUser = null)
    {
        FirebaseUser fUser = targetUser ?? user;
        if (fUser == null) {
            if (statusText) statusText.text = "Firebase: Pas connecte.";
            return;
        }

        fUser.TokenAsync(true).ContinueWith(task => {
            if (task.IsCompletedSuccessfully) {
                string idToken = task.Result;
                UnityMainThreadDispatcher.Instance().Enqueue(() => {
                    networkClient.ConnectWithSocialLogin("firebase", fUser.UserId, idToken);
                    if (statusText) statusText.text = "Firebase: Connexion serveur...";
                });
            }
        });
    }

    private void OnDestroy() {
        if (auth != null) {
            auth.StateChanged -= AuthStateChanged;
        }
    }
}
