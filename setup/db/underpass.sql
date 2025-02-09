--
-- PostgreSQL database dump
--

SET statement_timeout = 0;
SET lock_timeout = 0;
SET idle_in_transaction_session_timeout = 0;
SET client_encoding = 'UTF8';
SET standard_conforming_strings = on;
SELECT pg_catalog.set_config('search_path', '', false);
SET check_function_bodies = false;
SET xmloption = content;
SET client_min_messages = warning;
SET row_security = off;

CREATE EXTENSION IF NOT EXISTS hstore WITH SCHEMA public;
COMMENT ON EXTENSION hstore IS 'data type for storing sets of (key, value) pairs';

CREATE EXTENSION IF NOT EXISTS postgis WITH SCHEMA public;
COMMENT ON EXTENSION postgis IS 'PostGIS geometry, geography, and raster spatial types and functions';

SET default_tablespace = '';

CREATE TABLE IF NOT EXISTS public.changesets (
    id int8 NOT NULL,
    editor text,
    uid integer NOT NULL,
    created_at timestamptz,
    closed_at timestamptz,
    updated_at timestamptz,
    hashtags text[],
    source text,
    bbox public.geometry(MultiPolygon,4326)
);

DROP TYPE IF EXISTS public.objtype;
CREATE TYPE public.objtype AS ENUM ('node', 'way', 'relation');

CREATE TABLE IF NOT EXISTS public.ways_poly (
    osm_id int8,
    changeset int8,
    geom public.geometry(Polygon,4326),
    tags JSONB,
    refs int8[],
    timestamp timestamp with time zone,
    version int,
    "user" text,
    uid int8
);

CREATE TABLE IF NOT EXISTS public.ways_line (
    osm_id int8,
    changeset int8,
    geom public.geometry(LineString,4326),
    tags JSONB,
    refs int8[],
    timestamp timestamp with time zone,
    version int,
    "user" text,
    uid int8
);

CREATE TABLE IF NOT EXISTS public.nodes (
    osm_id int8,
    changeset int8,
    geom public.geometry(Point,4326),
    tags JSONB,
    timestamp timestamp with time zone,
    version int,
    "user" text,
    uid int8
);

CREATE TABLE IF NOT EXISTS public.relations (
    osm_id int8,
    changeset int8,
    geom public.geometry(Geometry,4326),
    tags JSONB,
    refs JSONB,
    timestamp timestamp with time zone,
    version int,
    "user" text,
    uid int8
);

ALTER TABLE ONLY public.ways_poly
    ADD CONSTRAINT ways_poly_pkey PRIMARY KEY (osm_id);

ALTER TABLE ONLY public.ways_line
    ADD CONSTRAINT ways_line_pkey PRIMARY KEY (osm_id);

ALTER TABLE ONLY public.nodes
    ADD CONSTRAINT nodes_pkey PRIMARY KEY (osm_id);

ALTER TABLE ONLY public.relations
    ADD CONSTRAINT relations_pkey PRIMARY KEY (osm_id);

ALTER TABLE ONLY public.changesets
    ADD CONSTRAINT changesets_pkey PRIMARY KEY (id);
