# Using the bootstrap.sh script

This script is prepared for a quick bootstrap of the Underpass database.

## Install requirements

- fiona `pip install fiona`
- shapely `pip install shapely`
- osm2pgsql (https://osm2pgsql.org/doc/install.html)
- psql (https://www.postgresql.org/download/)

## Quick start

Run the script passing region, country and DB username as arguments, for example:

```bash
./bootstrap.sh -r south-america -c ecuador -l yes
```

This will:

1. Delete all entries into the database (WARNING: POTENTIAL LOSS OF DATA)
2. Download raw data from GeoFabrik
3. Run osm2psql for import it
4. Convert the country .poly file to .geojson and install config files
5. Run underpass --bootstrap for bootstrapping the validation table

### Script options

```
-r region (Region for bootstrapping)
   africa, asia, australia, central-america
   europe, north-america or south-america
-c country (Country inside the region)
-h host (Database host)
-u user (Database user)
-p port (Database port)
-d database (Database name)
-l yes (Use local files instead of download them)
```

Use the `-l yes` when you have your .pbf and .poly files already downloaded and
you don't want to download them again. The script will look for those files
using the `-r` and `-c` arguments, for example

```bash
./bootstrap.sh -r south-america -c ecuador -u underpass -l yes
```

Will look for these files:

```
ecuador-latests.osm.pbf
ecuador.poly
```

## OSM authentication for downloading data

If you want to bootstrap your database with ChangeSet data downloaded from
 GeoFabrik, you'll need to be authenticated with an OSM account.

 A good utility for doing this from command line is in the sendfile_osm_oauth_protector
 repository.

 ```bash
 git clone https://github.com/geofabrik/sendfile_osm_oauth_protector.git
 cd sendfile_osm_oauth_protector
 ```

Edit `settings.json` with your credentials:

 ```json
 {
  "user": "<your_osm_username>",
  "password": "<your_osm_password>",
  "osm_host": "https://www.openstreetmap.org",
  "consumer_url": "https://osm-internal.download.geofabrik.de/get_cookie"
}
```

And download your file:

```bash
curl https://osm-internal.download.geofabrik.de/africa/tanzania-latest-internal.osm.pbf \
   --cookie "$(cat cookie_output_file.txt)" --output tanzania-latest.osm.pbf
```

Then you can move the pbf file to the `underpass/utils` directory and use the `-l yes` option.
