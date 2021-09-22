﻿using System;
using System.Linq;
using System.Threading.Tasks;
using Azure;
using Azure.Data.Tables;
using IoTalkie.Common;
using Microsoft.Extensions.Logging;
using Microsoft.Extensions.Options;

namespace IoTalkie.Messaging.Channel.Devices
{
    public class DeviceRegistry
    {
        private readonly ILogger<DeviceRegistry> _logger;
        private readonly AzureSettings _settings;

        public class DeviceAssignmentEntity : ITableEntity
        {
            public string PartitionKey { get; set; }
            public string RowKey { get; set; }
            public DateTimeOffset? Timestamp { get; set; }
            public ETag ETag { get; set; }
            
            public string DeviceId { get; set; }

            public string UserId { get; set; }
        }

        public DeviceRegistry(IOptions<AzureSettings> settings, ILogger<DeviceRegistry> logger)
        {
            _logger = logger;
            _settings = settings.Value;
        }

        public async Task<ContactPrincipal> GetOwner(DevicePrincipal principal)
        {
            var client = new TableClient(_settings.BlobStorageConnectionString, "DeviceAssignments");
            await client.CreateIfNotExistsAsync();

            var result = client.Query<DeviceAssignmentEntity>(ent => ent.DeviceId == principal.ClientId);

            if (!result.Any())
            {
                _logger.LogError($"Unable to find owner user for device with DeviceId/ClientId '{principal.ClientId}'");
                return null;
            }

            var userId = result.AsPages().First().Values.First().UserId;

            return new ContactPrincipal(userId, principal);
        }
    }
}