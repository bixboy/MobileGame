using UnityEngine;
using System;
using System.Net;
using System.Net.Http;
using System.Collections.Generic;
using System.Threading.Tasks;
using System.IO;
using System.Text;

public class OAuthDesktopFlow : MonoBehaviour
{
    private const string AuthorizationEndpoint = "https://accounts.google.com/o/oauth2/v2/auth";
    private const string TokenEndpoint = "https://oauth2.googleapis.com/token";

    // Les scopes nécessaires
    private const string Scopes = "openid email profile";

    public async Task<string> GetIdTokenAsync(string clientId, string clientSecret)
    {
        // 1. Définir le Redirect URI (127.0.0.1 est souvent plus compatible que localhost)
        string redirectUri = "http://127.0.0.1:5001/"; 
        
        // 2. Créer l'URL d'autorisation
        string state = Guid.NewGuid().ToString();
        string authorizationRequest = $"{AuthorizationEndpoint}?response_type=code" +
                                      $"&client_id={clientId}" +
                                      $"&redirect_uri={Uri.EscapeDataString(redirectUri)}" +
                                      $"&scope={Uri.EscapeDataString(Scopes)}" +
                                      $"&state={state}";

        // 3. Lancer l'écouteur local
        using (var httpListener = new HttpListener())
        {
            httpListener.Prefixes.Add(redirectUri);
            httpListener.Start();

            Debug.Log($"[OAuth] Ouvrant le navigateur : {authorizationRequest}");
            Application.OpenURL(authorizationRequest);

            // 4. Attendre la réponse de Google
            var context = await httpListener.GetContextAsync();
            var response = context.Response;

            string code = context.Request.QueryString.Get("code");
            string incomingState = context.Request.QueryString.Get("state");

            // Réponse visuelle dans le navigateur
            string responseString = "<html><body><h1>Authentification reussie !</h1><p>Vous pouvez fermer cette fenêtre et retourner au jeu.</p></body></html>";
            var buffer = Encoding.UTF8.GetBytes(responseString);
            response.ContentLength64 = buffer.Length;
            response.OutputStream.Write(buffer, 0, buffer.Length);
            response.OutputStream.Close();
            httpListener.Stop();

            if (incomingState != state)
            {
                Debug.LogError("[OAuth] Erreur de validation du State.");
                return null;
            }

            if (string.IsNullOrEmpty(code))
            {
                Debug.LogError("[OAuth] Aucun code reçu.");
                return null;
            }

            // 5. Échanger le Code contre un ID Token via HTTP
            return await ExchangeCodeForIdTokenAsync(clientId, clientSecret, code, redirectUri);
        }
    }

    private async Task<string> ExchangeCodeForIdTokenAsync(string clientId, string clientSecret, string code, string redirectUri)
    {
        Debug.Log("[OAuth] Echange du code contre un token...");
        
        using (var client = new HttpClient())
        {
            var values = new Dictionary<string, string>
            {
                { "code", code },
                { "client_id", clientId },
                { "client_secret", clientSecret },
                { "redirect_uri", redirectUri },
                { "grant_type", "authorization_code" }
            };

            var content = new FormUrlEncodedContent(values);
            var response = await client.PostAsync(TokenEndpoint, content);
            var responseString = await response.Content.ReadAsStringAsync();

            Debug.Log($"[OAuth] Reponse Google: {response.StatusCode}");

            if (response.IsSuccessStatusCode)
            {
                Debug.Log($"[OAuth] Raw JSON: {responseString}");
                string idToken = ExtractValueFromJson(responseString, "id_token");
                Debug.Log($"[OAuth] Token ID extrait: {(string.IsNullOrEmpty(idToken) ? "VIDE" : "OK (" + idToken.Length + " chars)")}");
                return idToken;
            }
            else
            {
                Debug.LogError($"[OAuth] Erreur d'échange : {responseString}");
                return null;
            }
        }
    }

    private string ExtractValueFromJson(string json, string key)
    {
        // On cherche "key"
        int keyIndex = json.IndexOf($"\"{key}\"");
        if (keyIndex == -1) return null;

        // On cherche le début de la valeur après le ":"
        int colonIndex = json.IndexOf(":", keyIndex);
        if (colonIndex == -1) return null;

        int valueStart = json.IndexOf("\"", colonIndex);
        if (valueStart == -1) return null;
        valueStart++; // On saute le guillemet ouvrant

        int valueEnd = json.IndexOf("\"", valueStart);
        if (valueEnd == -1) return null;

        return json.Substring(valueStart, valueEnd - valueStart);
    }
}
