﻿namespace HttpReverseProxy.Plugin.InjectFile
{
  using HttpReverseProxyLib;
  using HttpReverseProxyLib.DataTypes.Class;
  using HttpReverseProxyLib.DataTypes.Enum;
  using HttpReverseProxyLib.Exceptions;


  public partial class InjectFile
  {

    /// <summary>
    ///
    /// </summary>
    /// <param name="pluginHost"></param>
    public void OnPostServerDataResponse(RequestObj requestObj, DataPacket dataPacket)
    {
      if (requestObj == null)
      {
        throw new ProxyWarningException("The request object is invalid");
      }

      if (dataPacket == null)
      {
        throw new ProxyWarningException("The request object is invalid");
      }

      Logging.Instance.LogMessage(requestObj.Id, ProxyProtocol.Undefined, Loglevel.Debug, "InjectFile.OnPostServerDataResponse(): ");
    }
  }
}