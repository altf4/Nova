function updateConfiguration()
{
  if(message.id == '')
  {
    alert('Attemping to submit to null target string, doing nothing');
    return;
  }
  
  now.UpdateConfiguration();
}
 
now.UpdateConfiguration = function()
{
  //construct json here
  //message type: updateConfiguration
  message.type = 'updateConfiguration';
  
  message.TCP_TIMEOUT = document.getElementsByName('TCP_TIMEOUT')[0].value;
  message.TCP_CHECK_FREQ = document.getElementsByName('TCP_CHECK_FREQ')[0].value;
  message.CLASSIFICATION_TIMEOUT = document.getElementsByName('CLASSIFICATION_TIMEOUT')[0].value;
  message.K = document.getElementsByName('K')[0].value;
  message.EPS = document.getElementsByName('EPS')[0].value;
  message.CLASSIFICATION_THRESHOLD = document.getElementsByName('CLASSIFICATION_THRESHOLD')[0].value;
  message.MIN_PACKET_THRESHOLD = document.getElementsByName('MIN_PACKET_THRESHOLD')[0].value;
  
  if(document.getElementById('clearHostileYes').checked)
  {
    message.CLEAR_AFTER_HOSTILE_EVENT = '1';  
  }
  else
  {
    message.CLEAR_AFTER_HOSTILE_EVENT = '0';
  }
  
  message.CUSTOM_PCAP_FILTER = document.getElementsByName('CUSTOM_PCAP_FILTER')[0].value;
  
  if(document.getElementById('customPcapYes').checked)
  {
    message.CUSTOM_PCAP_MODE = '1';
  }
  else
  {
    message.CUSTOM_PCAP_MODE = '0';
  }
  
  message.CAPTURE_BUFFER_SIZE = document.getElementsByName('CAPTURE_BUFFER_SIZE')[0].value;
  message.SAVE_FREQUENCY = document.getElementsByName('SAVE_FREQUENCY')[0].value;
  message.DATA_TTL = document.getElementsByName('DATA_TTL')[0].value;
  
  if(document.getElementById('dmEnabledYes').checked)
  {
    message.DM_ENABLED = '1';
  }
  else
  {
    message.DM_ENABLED = '0';
  }
  
  message.SERVICE_PREFERENCES = document.getElementsByName('SERVICE_PREFERENCES')[0].value;
  
  if((/^0:[0-7](\+|\-)?;1:[0-7](\+|\-)?;2:[0-7](\+|\-)?;$/).test(message.SERVICE_PREFERENCES) == false)
  {
    document.getElementById('SERVICE_PREFERENCES').value = replace;
    alert('Service Preferences string is not formatted correctly.');
    return;
  }
  
  now.UpdateBaseConfig(message);
  now.MessageSend(message);
  location.reload(true);
}