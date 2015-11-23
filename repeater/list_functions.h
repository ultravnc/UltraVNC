

void Add_viewer_list(mystruct *Viewerstruct);
ULONG  Find_server_list(mystruct *Serverstruct);
ULONG Find_viewer_list(mystruct *Viewerstruct);
ULONG Find_viewer_list_id(mystruct *Viewerstruct);
void Remove_viewer_list(ULONG code);
void Remove_server_list(ULONG code);
void Add_server_list(mystruct *Serverstruct);
ULONG Find_server_list_id(mystruct *Serverstruct);
void Clean_server_List();
void Clean_viewer_List();
void Start_cleaupthread();
void Stop_cleaupthread();
