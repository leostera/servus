let version = "0.1";

module File = {
  let read = path =>
    switch (Sys.file_exists(path)) {
    | true =>
      switch (path |> Fpath.v |> Bos.OS.File.read) {
      | Ok(contents) => Some(contents)
      | _ => None
      }
    | _ => None
    };
};

let request_handler = (_client, req_desc) => {
  open Httpaf;
  let req = req_desc |> Reqd.request;

  switch (req.meth) {
  | `GET =>
    Logs.app(m => m("Request file: %s", req.target));

    let file = File.read(Filename.concat(".", req.target));

    switch (file) {
    | Some(contents) =>
      let content_len = contents |> String.length |> string_of_int;
      let headers =
        Headers.of_list([
          ("Content-Length", content_len),
          ("Content-Type", req.target |> Magic_mime.lookup),
        ]);
      let res = Response.create(`OK, ~headers);
      Reqd.respond_with_string(req_desc, res, contents);
    | None =>
      let res = Response.create(`Not_found);
      Reqd.respond_with_string(req_desc, res, "");
    };
  | _ =>
    let headers = Headers.of_list([("Content-Length", "0")]);
    let res = Response.create(`Method_not_allowed, ~headers);
    Reqd.respond_with_string(req_desc, res, "");
  };
};

let error_handler = (client, ~request=?, error, fn) => {
  client |> ignore;
  request |> ignore;
  error |> ignore;
  fn |> ignore;
};

let connection_handler =
  Httpaf_lwt.Server.create_connection_handler(
    ~request_handler,
    ~error_handler,
  );

let serve = (_flags, path, port) => {
  open Lwt.Infix;

  let listening_address = Unix.(ADDR_INET(inet_addr_any, port));

  Lwt_io.establish_server_with_client_socket(
    listening_address,
    connection_handler,
  )
  >>= (
    _server =>
      Logs_lwt.app(m => m("Running on port: %d", port))
      >>= (_ => Logs_lwt.app(m => m("Serving from path: %s", path)))
  )
  |> ignore;

  let (forever, _) = Lwt.wait();
  forever |> Lwt_main.run;
};
