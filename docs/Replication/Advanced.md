## Replication advanced options

You might run the `underpass` command with the following options:

```
  -h [ --help ]            display help
  -s [ --server ] arg      Database server for replicator output (defaults to
                           localhost/underpass) can be a hostname or a full
                           connection string USER:PASSSWORD@HOST/DATABASENAME
  -p [ --planet ] arg      Replication server (defaults to planet.maps.mail.ru)
  -u [ --url ] arg         Starting URL path (ex. 000/075/000), takes
                           precedence over 'timestamp' option
  --changeseturl arg       Starting URL path for ChangeSet (ex. 000/075/000),
                           takes precedence over 'timestamp' option
  -f [ --frequency ] arg   Update frequency (hourly, daily), default minutely)
  -t [ --timestamp ] arg   Starting timestamp (can be used 2 times to set a
                           range)
  -b [ --boundary ] arg    Boundary polygon file name
  --osmnoboundary          Disable boundary polygon for OsmChanges
  --oscnoboundary          Disable boundary polygon for Changesets
  --datadir arg            Directory for remote and local cached files (with
                           ending slash)
  --destdir_base arg       Base directory for local cached files (with ending
                           slash)
  -v [ --verbose ]         Enable verbosity
  -l [ --logstdout ]       Enable logging to stdout, default is log to
                           underpass.log
  --changefile arg         Import change file
  -c [ --concurrency ] arg Concurrency
  --changesets             Changesets only
  --osmchanges             OsmChanges only
  -d [ --debug ]           Enable debug messages for developers
  --norefs                 Disable refs (useful for non OSM data)
  --config                 Dump config
  -i [ --import ] arg      Initialize OSM database with OSM PBF datafile
  --silent                 Silent
```

