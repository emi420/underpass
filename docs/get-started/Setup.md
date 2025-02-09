## Boostraping the database

You can prepare your Underpass installation with data for a specific country.

### Pre-requisites

#### Database

Prepare your PostgreSQL + PostGIS database, for example:

```
sudo apt update 
sudo apt install postgis
sudo su - postgres
psql
postgres=# CREATE USER underpass WITH PASSWORD 'your_password';
postgres=# CREATE DATABASE underpass;
postgres=# GRANT ALL PRIVILEGES ON DATABASE "underpass" to underpass;
postgres=# ALTER ROLE underpass SUPERUSER;
postgres=# exit
exit
psql postgresql://underpass:your_password@localhost:5432/underpass < setup/db/underpass.sql
```

#### Requirements

```
sudo apt install python3-pip -y
sudo apt install python3.11-venv
python3 -m venv ~/venv
source ~/venv/bin/activate
pip install fiona
pip install shapely
apt install osm2pgsql
```

### Setup DB

1. Import `setup/db/underpass.sql` to a PostGIS database
2. Download a OSM OBF file, ex: `wget https://download.geofabrik.de/south-america/uruguay-latest.osm.pbf`
3. Import it `underpass -i uruguay-latest.osm.pbf`
4. Create indexes importing `setup/db/indexes.sql` the DB

### Setup priority area

If you already have a GeoJSON, skip steps 1 and 2.

1. Download a poly file, ex: `wget https://download.geofabrik.de/south-america/uruguay.poly`
2. Convert it to GeoJSON, ex: `python utils/poly2geojson.py uruguay.poly`
3. Copy priority file, ex: `cp uruguay.poly /usr/local/etc/underpass/priority.geojson`
