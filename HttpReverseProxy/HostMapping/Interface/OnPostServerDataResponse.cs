﻿namespace HttpReverseProxy.Plugin.HostMapping
{
  using HttpReverseProxyLib;
  using HttpReverseProxyLib.DataTypes;
  using HttpReverseProxyLib.Exceptions;
  using System.Collections.Concurrent;

  public partial class HostMapping
  {

    /// <summary>
    ///
    /// </summary>
    public void OnPostServerDataResponse(RequestObj requestObj, DataPacket dataPacket)
    {
      if (requestObj == null)
      {
        throw new ProxyWarningException("The request object is invalid");
      }

      if (requestObj.ServerResponseMetaDataObj == null)
      {
        throw new ProxyWarningException("The meta data object is invalid");
      }

      if (dataPacket == null)
      {
        throw new ProxyWarningException("The data packet is invalid");
      }

      if (string.IsNullOrEmpty(requestObj.ServerResponseMetaDataObj.ContentTypeEncoding.ContentType))
      {
        throw new ProxyWarningException("The server response content type is invalid");
      }
      
    }
  }
}