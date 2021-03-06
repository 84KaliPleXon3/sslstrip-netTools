﻿namespace HttpReverseProxy.Plugin.SslStrip
{
  using HttpReverseProxyLib.DataTypes.Enum;
  using HttpReverseProxyLib.DataTypes.Interface;
  using System;
  using System.IO;
  using SslStripConfig = HttpReverseProxy.Plugin.SslStrip.Config;


  public partial class SslStrip
  {

    /// <summary>
    ///
    /// </summary>
    /// <returns></returns>
    public void OnLoad(IPluginHost pluginHost)
    {
      // Set plugin properties
      this.pluginProperties.PluginHost = pluginHost;

      // Parse configuration file
      this.configurationFileFullPath = Path.Combine(this.pluginProperties.PluginDirectory, SslStripConfig.ConfigFileName);
      if (string.IsNullOrEmpty(this.configurationFileFullPath))
      {
        return;
      }

      try
      {
        this.PluginConfig.ParseConfigurationFile(this.configurationFileFullPath);
        foreach (var mimeType in Plugin.SslStrip.Config.PatternsPerMimetypeHost.Keys)
        {
          this.pluginProperties.PluginHost.LoggingInst.LogMessage("SslStrip", ProxyProtocol.Undefined, Loglevel.Info, "SslStrip.OnLoad(): MimeType {0} has {1} records", mimeType, Plugin.SslStrip.Config.PatternsPerMimetypeHost[mimeType].Count);
        }
      }
      catch (FileNotFoundException)
      {
        string tmpConfigFile = Path.GetFileName(this.configurationFileFullPath);
        this.pluginProperties.PluginHost.LoggingInst.LogMessage("SslStrip", ProxyProtocol.Undefined, Loglevel.Info, "SslStrip.OnLoad(): Config file \"...{0}\" does not exist", tmpConfigFile);
      }
      catch (Exception ex)
      {
        this.pluginProperties.PluginHost.LoggingInst.LogMessage("SslStrip", ProxyProtocol.Undefined, Loglevel.Info, "SslStrip.OnLoad(EXCEPTION): {0}", ex.Message);
      }

      this.pluginProperties.PluginHost.RegisterPlugin(this);
    }
  }
}
