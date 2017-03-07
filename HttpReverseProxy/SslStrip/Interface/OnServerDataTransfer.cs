﻿namespace HttpReverseProxy.Plugin.SslStrip
{
  using HttpReverseProxyLib;
  using HttpReverseProxyLib.DataTypes.Class;
  using HttpReverseProxyLib.DataTypes.Enum;
  using HttpReverseProxyLib.Exceptions;
  using System.Collections.Concurrent;


  public partial class SslStrip
  {

    /// <summary>
    /// 
    /// </summary>
    /// <param name="requestObj"></param>
    /// <param name="dataChunk"></param>
    public void OnServerDataTransfer(RequestObj requestObj, DataChunk dataChunk)
    {
      if (requestObj == null)
      {
        throw new ProxyWarningException("The request object is invalid");
      }

      if (requestObj.ServerResponseObj == null)
      {
        throw new ProxyWarningException("The meta data object is invalid");
      }

      if (dataChunk == null)
      {
        throw new ProxyWarningException("The data packet is invalid");
      }

      if (string.IsNullOrEmpty(requestObj.ServerResponseObj.ContentTypeEncoding.ContentType))
      {
        throw new ProxyWarningException("The server response content type is invalid");
      }

      if (!this.pluginConfig.SearchPatterns.ContainsKey(requestObj.ServerResponseObj.ContentTypeEncoding.ContentType))
      {
        return;
      }

      // 1. The content type is right.
      // Iterate through all configured tags and locate
      // them in the server data
      ConcurrentDictionary<string, string> foundHttpsTags = new ConcurrentDictionary<string, string>();
      ConcurrentDictionary<string, string> cacheUrlMapping = new ConcurrentDictionary<string, string>();
      string strippedData = string.Empty;

      this.sslStrippedData = dataChunk.DataEncoding.GetString(dataChunk.ContentData, 0, dataChunk.ContentData.Length);
      this.LocateAllTags(this.sslStrippedData, this.pluginConfig.SearchPatterns[requestObj.ServerResponseObj.ContentTypeEncoding.ContentType], foundHttpsTags, cacheUrlMapping);

      foreach (string tmpKey in foundHttpsTags.Keys)
      {
        Logging.Instance.LogMessage(requestObj.Id, ProxyProtocol.Undefined, Loglevel.Debug, "SslStrip.FoundHttpsHosts(): {0}: {1}", tmpKey, foundHttpsTags[tmpKey]);
      }

      // 2. Replace previously determined tags by the according replacement tag
      this.sslStrippedData = this.ReplaceRelevantTags(requestObj, this.sslStrippedData, foundHttpsTags);

      // 3. Encode content back to the charset the server reported (or default encoding)
      dataChunk.ContentData = requestObj.ServerResponseObj.ContentTypeEncoding.ContentCharsetEncoding.GetBytes(this.sslStrippedData);

      // 4. Keep SSL stripped URLs in cache
      foreach (string tmpKey in cacheUrlMapping.Keys)
      {
        Cache.CacheSslStrip.Instance.AddElement(requestObj.Id, tmpKey, cacheUrlMapping[tmpKey]);
      }
    }
  }
}