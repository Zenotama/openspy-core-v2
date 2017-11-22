#include <OS/OpenSpy.h>
#include <OS/Auth.h>
#include <curl/curl.h>
#include <jansson.h>
#include <string>


namespace OS {
	OS::TaskPool<AuthTask, AuthRequest> *m_auth_task_pool = NULL;
	struct curl_data {
	    std::string buffer;
	};
	/* callback for curl fetch */
	size_t AuthTask::curl_callback (void *contents, size_t size, size_t nmemb, void *userp) {
		if (!contents) {
			return 0;
		}
		size_t realsize = size * nmemb;                             /* calculate buffer size */
		curl_data *data = (curl_data *)userp;
		data->buffer += (const char *)contents;
		return realsize;
	}
	void AuthTask::PerformAuth_NickEMail_GPHash(AuthRequest request) {
		curl_data recv_data;
		//build json object
		json_t *send_obj = json_object();

		if(request.nick.length())
			json_object_set_new(send_obj, "profilenick", json_string(request.nick.c_str()));
		if(request.uniquenick.length())
			json_object_set_new(send_obj, "uniquenick", json_string(request.uniquenick.c_str()));

		if(request.email.length())
			json_object_set_new(send_obj, "email", json_string(request.email.c_str()));

		json_object_set_new(send_obj, "partnercode", json_integer(request.partnercode));
		json_object_set_new(send_obj, "namespaceid", json_integer(request.namespaceid));
		
		if(request.password.length())
			json_object_set_new(send_obj, "password", json_string(request.password.c_str()));

		json_object_set_new(send_obj, "hash_type", json_string("gp_nick_email"));
		json_object_set_new(send_obj, "set_context", json_string("profile"));
		json_object_set_new(send_obj, "save_session", request.create_session ? json_true() : json_false());


		json_object_set_new(send_obj, "server_challenge", json_string(request.server_challenge.c_str()));
		json_object_set_new(send_obj, "client_challenge", json_string(request.client_challenge.c_str()));
		json_object_set_new(send_obj, "client_response", json_string(request.client_response.c_str()));


		char *json_data_str = json_dumps(send_obj, 0);
		
		CURL *curl = curl_easy_init();
		CURLcode res;
		Profile profile;
		User user;
		user.id = 0;
		profile.id = 0;
		OS::AuthData auth_data;

		auth_data.response_code = (AuthResponseCode)-1;
		if(curl) {
			curl_easy_setopt(curl, CURLOPT_URL, OPENSPY_AUTH_URL);
			curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_data_str);

			/* set default user agent */
			curl_easy_setopt(curl, CURLOPT_USERAGENT, "OSCoreAuth");

			/* set timeout */
			curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5);

			/* enable location redirects */
			curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1);

			/* set maximum allowed redirects */
			curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 1);

			curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_callback);
			curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *) &recv_data);

			res = curl_easy_perform(curl);

			bool success = false;			

			if(res == CURLE_OK) {
				

				json_t *json_data = json_loads(recv_data.buffer.c_str(), 0, NULL);

				if(json_data) {
					json_t *success_obj = json_object_get(json_data, "success");
					if(success_obj == json_true()) {
						json_t *profile_json = json_object_get(json_data, "profile");
						if(profile_json) {
							profile = LoadProfileFromJson(profile_json);
							json_t *user_json = json_object_get(profile_json, "user");
							if(user_json) {
								user = LoadUserFromJson(user_json);
								success = true;
							}
						}
						json_t *server_response_json = json_object_get(json_data, "server_response");
						if(server_response_json) {
							auth_data.hash_proof = json_string_value(server_response_json);
						}
						server_response_json = json_object_get(json_data, "session_key");
						if(server_response_json) {
							auth_data.session_key = json_string_value(server_response_json);
						}

					}
					json_t *reason_json = json_object_get(json_data, "reason");
					if(reason_json) {
						auth_data.response_code = (AuthResponseCode)json_integer_value(reason_json);
					}
					json_decref(json_data);
				} else {
					success = false;
					auth_data.response_code = LOGIN_RESPONSE_SERVER_ERROR;
				}

				request.callback(success, user, profile, auth_data, request.extra, request.operation_id, request.peer);
			}
			curl_easy_cleanup(curl);
		}

		if(json_data_str)
			free((void *)json_data_str);

		json_decref(send_obj);
	}
	void AuthTask::PerformAuth_PreAuth_Token(AuthRequest request) {
		curl_data recv_data;
		//build json object
		json_t *send_obj = json_object();

		if (request.nick.length())
			json_object_set_new(send_obj, "profilenick", json_string(request.nick.c_str()));
		if (request.uniquenick.length())
			json_object_set_new(send_obj, "uniquenick", json_string(request.uniquenick.c_str()));

		if (request.email.length())
			json_object_set_new(send_obj, "email", json_string(request.email.c_str()));

		if (request.password.length())
			json_object_set_new(send_obj, "auth_token", json_string(request.password.c_str()));

		json_object_set_new(send_obj, "server_challenge", json_string(request.server_challenge.c_str()));
		json_object_set_new(send_obj, "client_challenge", json_string(request.client_challenge.c_str()));
		json_object_set_new(send_obj, "client_response", json_string(request.client_response.c_str()));

		json_object_set_new(send_obj, "hash_type", json_string("gp_preauth"));
		json_object_set_new(send_obj, "set_context", json_string("profile"));
		json_object_set_new(send_obj, "save_session", request.create_session ? json_true() : json_false());


		char *json_data = json_dumps(send_obj, 0);

		CURL *curl = curl_easy_init();
		CURLcode res;
		Profile profile;
		User user;
		user.id = 0;
		profile.id = 0;
		OS::AuthData auth_data;

		auth_data.response_code = (AuthResponseCode)-1;
		if (curl) {
			curl_easy_setopt(curl, CURLOPT_URL, OPENSPY_AUTH_URL);
			curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_data);

			/* set default user agent */
			curl_easy_setopt(curl, CURLOPT_USERAGENT, "OSCoreAuth");

			/* set timeout */
			curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5);

			/* enable location redirects */
			curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1);

			/* set maximum allowed redirects */
			curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 1);

			curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_callback);
			curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&recv_data);

			res = curl_easy_perform(curl);

			bool success = false;

			if (res == CURLE_OK) {

				json_t *json_data = json_loads(recv_data.buffer.c_str(), 0, NULL);


				if (json_data) {
					json_t *success_obj = json_object_get(json_data, "success");
					if (success_obj == json_true()) {
						json_t *profile_json = json_object_get(json_data, "profile");
						if (profile_json) {
							profile = LoadProfileFromJson(profile_json);
							json_t *user_json = json_object_get(profile_json, "user");
							if (user_json) {
								user = LoadUserFromJson(user_json);
								success = true;
							}
						}
						json_t *server_response_json = json_object_get(json_data, "session_key");
						if (server_response_json) {
							auth_data.session_key = json_string_value(server_response_json);
						}
						server_response_json = json_object_get(json_data, "server_response");
						if (server_response_json) {
							auth_data.hash_proof = json_string_value(server_response_json);
						}

					}
					else {
					}
					json_t *reason_json = json_object_get(json_data, "reason");
					if (reason_json) {
						auth_data.response_code = (AuthResponseCode)json_integer_value(reason_json);
					}
					json_decref(json_data);
				}
				else {
					success = false;
					auth_data.response_code = LOGIN_RESPONSE_SERVER_ERROR;
				}
				request.callback(success, user, profile, auth_data, request.extra, request.operation_id, request.peer);
			}
			curl_easy_cleanup(curl);
		}
		json_decref(send_obj);
	}
	void AuthTask::PerformAuth_NickEMail(AuthRequest request) {
		curl_data recv_data;
		//build json object
		json_t *send_obj = json_object();

		if(request.nick.length())
			json_object_set_new(send_obj, "profilenick", json_string(request.nick.c_str()));
		if(request.uniquenick.length())
			json_object_set_new(send_obj, "uniquenick", json_string(request.uniquenick.c_str()));

		if(request.email.length())
			json_object_set_new(send_obj, "email", json_string(request.email.c_str()));

		json_object_set_new(send_obj, "partnercode", json_integer(request.partnercode));
		json_object_set_new(send_obj, "namespaceid", json_integer(request.namespaceid));
		
		if(request.password.length())
			json_object_set_new(send_obj, "password", json_string(request.password.c_str()));

		json_object_set_new(send_obj, "hash_type", json_string("nick_email"));
		json_object_set_new(send_obj, "set_context", json_string("profile"));
		json_object_set_new(send_obj, "save_session", request.create_session ? json_true() : json_false());


		char *json_data = json_dumps(send_obj, 0);

		CURL *curl = curl_easy_init();
		CURLcode res;
		Profile profile;
		User user;
		user.id = 0;
		profile.id = 0;
		OS::AuthData auth_data;

		auth_data.response_code = (AuthResponseCode)-1;
		if(curl) {
			curl_easy_setopt(curl, CURLOPT_URL, OPENSPY_AUTH_URL);
			curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_data);

			/* set default user agent */
			curl_easy_setopt(curl, CURLOPT_USERAGENT, "OSCoreAuth");

			/* set timeout */
			curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5);

			/* enable location redirects */
			curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1);

			/* set maximum allowed redirects */
			curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 1);

			curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_callback);
			curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *) &recv_data);

			res = curl_easy_perform(curl);

			bool success = false;

			if(res == CURLE_OK) {

				json_t *json_data = json_loads(recv_data.buffer.c_str(), 0, NULL);


				if(json_data) {
					json_t *success_obj = json_object_get(json_data, "success");
					if(success_obj == json_true()) {
						json_t *profile_json = json_object_get(json_data, "profile");
						if(profile_json) {
							profile = LoadProfileFromJson(profile_json);
							json_t *user_json = json_object_get(profile_json, "user");
							if(user_json) {
								user = LoadUserFromJson(user_json);
								success = true;
							}
						}
						json_t *server_response_json = json_object_get(json_data, "session_key");
						if(server_response_json) {
							auth_data.session_key = json_string_value(server_response_json);
						}
					} else {
					}
					json_t *reason_json = json_object_get(json_data, "reason");
					if(reason_json) {
						auth_data.response_code = (AuthResponseCode)json_integer_value(reason_json);
					}
					json_decref(json_data);
				} else {
					success = false;
					auth_data.response_code = LOGIN_RESPONSE_SERVER_ERROR;
				}
				request.callback(success, user, profile, auth_data, request.extra, request.operation_id, request.peer);
			}
			curl_easy_cleanup(curl);
		}
		json_decref(send_obj);
	}
	void AuthTask::PerformAuth_CreateUser_OrProfile(AuthRequest request) {
		curl_data recv_data;
		//build json object
		json_t *send_obj = json_object();

		if(request.nick.length())
			json_object_set_new(send_obj, "profilenick", json_string(request.nick.c_str()));
		if(request.uniquenick.length())
			json_object_set_new(send_obj, "uniquenick", json_string(request.uniquenick.c_str()));

		if(request.email.length())
			json_object_set_new(send_obj, "email", json_string(request.email.c_str()));

		json_object_set_new(send_obj, "partnercode", json_integer(request.partnercode));
		json_object_set_new(send_obj, "namespaceid", json_integer(request.namespaceid));

		if(request.password.length())
			json_object_set_new(send_obj, "password", json_string(request.password.c_str()));

		json_object_set_new(send_obj, "hash_type", json_string("auth_or_create_profile"));
		json_object_set_new(send_obj, "set_context", json_string("profile"));
		json_object_set_new(send_obj, "save_session", request.create_session ? json_true() : json_false());


		char *json_data = json_dumps(send_obj, 0);

		CURL *curl = curl_easy_init();
		CURLcode res;
		Profile profile;
		User user;
		user.id = 0;
		profile.id = 0;
		OS::AuthData auth_data;

		auth_data.response_code = (AuthResponseCode)-1;
		if(curl) {
			curl_easy_setopt(curl, CURLOPT_URL, OPENSPY_AUTH_URL);
			curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_data);

			/* set default user agent */
			curl_easy_setopt(curl, CURLOPT_USERAGENT, "OSCoreAuth");

			/* set timeout */
			curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5);

			/* enable location redirects */
			curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1);

			/* set maximum allowed redirects */
			curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 1);

			curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_callback);
			curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *) &recv_data);

			res = curl_easy_perform(curl);

			bool success = false;

			if(res == CURLE_OK) {
				json_t *json_data = json_loads(recv_data.buffer.c_str(), 0, NULL);
				if(json_data) {
					json_t *reason_json = json_object_get(json_data, "reason");
					if(reason_json) {
						auth_data.response_code = (AuthResponseCode)json_integer_value(reason_json);
					} else {
						json_t *profile_json = json_object_get(json_data, "profile");
						json_t *user_json = json_object_get(profile_json, "user");
						user = LoadUserFromJson(user_json);
						profile = LoadProfileFromJson(profile_json);
						success = true;
					}
					json_decref(json_data);
				} else {
					success = false;
					auth_data.response_code = LOGIN_RESPONSE_SERVER_ERROR;
				}
				request.callback(success, user, profile, auth_data, request.extra, request.operation_id, request.peer);
			}
			curl_easy_cleanup(curl);
		}
		
		if(json_data)
			free(json_data);
		json_decref(send_obj);
	}
	void AuthTask::PerformAuth_MakeAuthTicket(AuthRequest request) {
		curl_data recv_data;
		//build json object
		json_t *send_obj = json_object();

		json_object_set_new(send_obj, "profileid", json_integer(request.profileid));

		if (request.password.length())
			json_object_set_new(send_obj, "password", json_string(request.password.c_str()));

		json_object_set_new(send_obj, "mode", json_string("make_auth_ticket"));
		json_object_set_new(send_obj, "set_context", json_string("profile"));
		json_object_set_new(send_obj, "save_session", request.create_session ? json_true() : json_false());

		json_object_set_new(send_obj, "session_key", json_integer(request.session_key));

		char *json_dump = json_dumps(send_obj, 0);

		CURL *curl = curl_easy_init();
		CURLcode res;
		Profile profile;
		User user;
		user.id = 0;
		profile.id = 0;
		OS::AuthData auth_data;

		auth_data.response_code = (AuthResponseCode)-1;


		if (curl) {

			curl_easy_setopt(curl, CURLOPT_URL, OPENSPY_AUTH_URL);
			curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_dump);

			/* set default user agent */
			curl_easy_setopt(curl, CURLOPT_USERAGENT, "OSCoreAuth");

			/* set timeout */
			curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5);

			/* enable location redirects */
			curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1);

			/* set maximum allowed redirects */
			curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 1);

			curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_callback);
			curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&recv_data);

			res = curl_easy_perform(curl);

			bool success = false;

			if (res == CURLE_OK) {
				json_t *json_data = json_loads(recv_data.buffer.c_str(), 0, NULL);
				if (json_data) {
					json_t *success_obj = json_object_get(json_data, "success");
					if (success_obj == json_true()) {
						success = true;
						json_t *session_key_json = json_object_get(json_data, "ticket");
						if (session_key_json) {
							auth_data.session_key = json_string_value(session_key_json);
						}

						session_key_json = json_object_get(json_data, "challenge");
						if (session_key_json) {
							auth_data.hash_proof = json_string_value(session_key_json);
						}
					}
					else {
					}
					json_t *reason_json = json_object_get(json_data, "reason");
					if (reason_json) {
						auth_data.response_code = (AuthResponseCode)json_integer_value(reason_json);
					}
					json_decref(json_data);
				}
				else {
					success = false;
					auth_data.response_code = LOGIN_RESPONSE_SERVER_ERROR;
				}
				request.callback(success, OS::User(), OS::Profile(), auth_data, request.extra, request.operation_id, request.peer);
			}
			curl_easy_cleanup(curl);
		}

		json_decref(send_obj);
	}
	void AuthTask::PerformAuth_PID_GSStats_SessKey(AuthRequest request) {
		curl_data recv_data;
		//build json object
		json_t *send_obj = json_object();

		json_object_set_new(send_obj, "profileid", json_integer(request.profileid));
		
		if(request.password.length())
			json_object_set_new(send_obj, "password", json_string(request.password.c_str()));

		json_object_set_new(send_obj, "hash_type", json_string("gstats_pid_sesskey"));
		json_object_set_new(send_obj, "set_context", json_string("profile"));
		json_object_set_new(send_obj, "save_session", request.create_session ? json_true() : json_false());


		json_object_set_new(send_obj, "client_response", json_string(request.client_response.c_str()));
		json_object_set_new(send_obj, "session_key", json_integer(request.session_key));


		char *json_dump = json_dumps(send_obj, 0);

		CURL *curl = curl_easy_init();
		CURLcode res;
		Profile profile;
		User user;
		user.id = 0;
		profile.id = 0;
		OS::AuthData auth_data;

		auth_data.response_code = (AuthResponseCode)-1;


		if(curl) {

			curl_easy_setopt(curl, CURLOPT_URL, OPENSPY_AUTH_URL);
			curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_dump);

			/* set default user agent */
			curl_easy_setopt(curl, CURLOPT_USERAGENT, "OSCoreAuth");

			/* set timeout */
			curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5);

			/* enable location redirects */
			curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1);

			/* set maximum allowed redirects */
			curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 1);

			curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_callback);
			curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *) &recv_data);

			res = curl_easy_perform(curl);

			bool success = false;

			if(res == CURLE_OK) {
				json_t *json_data = json_loads(recv_data.buffer.c_str(), 0, NULL);
				if(json_data) {
					json_t *success_obj = json_object_get(json_data, "success");
					if(success_obj == json_true()) {
						json_t *profile_json = json_object_get(json_data, "profile");
						if(profile_json) {
							profile = LoadProfileFromJson(profile_json);
							json_t *user_json = json_object_get(profile_json, "user");
							if(user_json) {
								user = LoadUserFromJson(user_json);
								success = true;
							}
						}
						json_t *session_key_json = json_object_get(json_data, "session_key");
						if(session_key_json) {
							auth_data.session_key = json_string_value(session_key_json);
						}

					} else {
					}
					json_t *reason_json = json_object_get(json_data, "reason");
					if(reason_json) {
						auth_data.response_code = (AuthResponseCode)json_integer_value(reason_json);
					}
					json_decref(json_data);
				} else {
					success = false;
					auth_data.response_code = LOGIN_RESPONSE_SERVER_ERROR;
				}

				request.callback(success, user, profile, auth_data, request.extra, request.operation_id, request.peer);
			}
			curl_easy_cleanup(curl);
		}

		json_decref(send_obj);
	}
	void AuthTask::PerformAuth_EmailPass(AuthRequest request) {
		curl_data recv_data;
		//build json object
		json_t *send_obj = json_object();

		json_object_set_new(send_obj, "email", json_string(request.email.c_str()));

		if (request.password.length())
			json_object_set_new(send_obj, "password", json_string(request.password.c_str()));

		json_object_set_new(send_obj, "hash_type", json_string("plain"));
		json_object_set_new(send_obj, "set_context", json_string("user"));
		json_object_set_new(send_obj, "save_session", request.create_session ? json_true() : json_false());

		json_object_set_new(send_obj, "session_key", json_integer(request.session_key));


		char *json_dump = json_dumps(send_obj, 0);

		CURL *curl = curl_easy_init();
		CURLcode res;
		Profile profile;
		User user;
		user.id = 0;
		profile.id = 0;
		OS::AuthData auth_data;

		auth_data.response_code = (AuthResponseCode)-1;


		if (curl) {

			curl_easy_setopt(curl, CURLOPT_URL, OPENSPY_AUTH_URL);
			curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_dump);

			/* set default user agent */
			curl_easy_setopt(curl, CURLOPT_USERAGENT, "OSCoreAuth");

			/* set timeout */
			curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5);

			/* enable location redirects */
			curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1);

			/* set maximum allowed redirects */
			curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 1);

			curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_callback);
			curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&recv_data);

			res = curl_easy_perform(curl);

			bool success = false;

			if (res == CURLE_OK) {
				json_t *json_data = json_loads(recv_data.buffer.c_str(), 0, NULL);
				if (json_data) {
					json_t *success_obj = json_object_get(json_data, "success");
					if (success_obj == json_true()) {
						json_t *user_json = json_object_get(json_data, "user");
						if (user_json) {
							user = LoadUserFromJson(user_json);
							success = true;
						}
						json_t *session_key_json = json_object_get(json_data, "session_key");
						if (session_key_json) {
							auth_data.session_key = json_string_value(session_key_json);
						}
					}
					else {
					}
					json_t *reason_json = json_object_get(json_data, "reason");
					if (reason_json) {
						auth_data.response_code = (AuthResponseCode)json_integer_value(reason_json);
					}
					json_decref(json_data);
				}
				else {
					success = false;
					auth_data.response_code = LOGIN_RESPONSE_SERVER_ERROR;
				}

				request.callback(success, user, OS::Profile(), auth_data, request.extra, request.operation_id, request.peer);
			}
			curl_easy_cleanup(curl);
		}

		json_decref(send_obj);
	}

	void AuthTask::PerformAuth_Uniquenick_Password(AuthRequest request) {
		curl_data recv_data;
		//build json object
		json_t *send_obj = json_object();

		json_object_set_new(send_obj, "uniquenick", json_string(request.uniquenick.c_str()));

		if (request.password.length())
			json_object_set_new(send_obj, "password", json_string(request.password.c_str()));

		if (request.partnercode != -1)
			json_object_set_new(send_obj, "partnercode", json_integer(request.partnercode));

		if (request.namespaceid != -1)
			json_object_set_new(send_obj, "namespaceid", json_integer(request.namespaceid));

		json_object_set_new(send_obj, "hash_type", json_string("plain"));
		json_object_set_new(send_obj, "set_context", json_string("profile"));
		json_object_set_new(send_obj, "save_session", request.create_session ? json_true() : json_false());

		json_object_set_new(send_obj, "session_key", json_integer(request.session_key));


		char *json_dump = json_dumps(send_obj, 0);

		CURL *curl = curl_easy_init();
		CURLcode res;
		Profile profile;
		User user;
		user.id = 0;
		profile.id = 0;
		OS::AuthData auth_data;

		auth_data.response_code = (AuthResponseCode)-1;


		if (curl) {

			curl_easy_setopt(curl, CURLOPT_URL, OPENSPY_AUTH_URL);
			curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_dump);

			/* set default user agent */
			curl_easy_setopt(curl, CURLOPT_USERAGENT, "OSCoreAuth");

			/* set timeout */
			curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5);

			/* enable location redirects */
			curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1);

			/* set maximum allowed redirects */
			curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 1);

			curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_callback);
			curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&recv_data);

			res = curl_easy_perform(curl);

			bool success = false;

			if (res == CURLE_OK) {
				json_t *json_data = json_loads(recv_data.buffer.c_str(), 0, NULL);
				if (json_data) {
					json_t *success_obj = json_object_get(json_data, "success");
					if (success_obj == json_true()) {
						json_t *profile_json = json_object_get(json_data, "profile");
						if (profile_json) {
							profile = LoadProfileFromJson(profile_json);
							json_t *user_json = json_object_get(profile_json, "user");
							if (user_json) {
								user = LoadUserFromJson(user_json);
								success = true;
							}
						}
						json_t *session_key_json = json_object_get(json_data, "session_key");
						if (session_key_json) {
							auth_data.session_key = json_string_value(session_key_json);
						}

					}
					else {
					}
					json_t *reason_json = json_object_get(json_data, "reason");
					if (reason_json) {
						auth_data.response_code = (AuthResponseCode)json_integer_value(reason_json);
					}
					json_decref(json_data);
				} else {
					success = false;
					auth_data.response_code = LOGIN_RESPONSE_SERVER_ERROR;
				}

				request.callback(success, user, profile, auth_data, request.extra, request.operation_id, request.peer);
			}
			curl_easy_cleanup(curl);
		}

		json_decref(send_obj);
	}
	void AuthTask::TryMakeAuthTicket(int profileid, AuthCallback cb, void *extra, int operation_id, INetPeer *peer) {
		AuthRequest request;
		request.type = EAuthType_MakeAuthTicket;
		request.profileid = profileid;
		request.session_key = 0;
		request.extra = extra;
		request.callback = cb;
		request.create_session = true;
		request.operation_id = operation_id;
		if (peer) {
			peer->IncRef();
		}
		request.peer = peer;
		m_auth_task_pool->AddRequest(request);
	}
	void AuthTask::TryAuthUniqueNick_Plain(std::string uniquenick, int partnercode, int namespaceid, std::string password, AuthCallback cb, void *extra, int operation_id, INetPeer *peer) {
		AuthRequest request;
		request.type = EAuthType_Uniquenick_Password;
		request.uniquenick = uniquenick;
		request.password = password;
		request.session_key = 0;
		request.namespaceid = namespaceid;
		request.partnercode = partnercode;
		request.extra = extra;
		request.callback = cb;
		request.create_session = true;
		request.operation_id = operation_id;
		if (peer) {
			peer->IncRef();
		}
		request.peer = peer;
		m_auth_task_pool->AddRequest(request);
	}
	void AuthTask::TryAuthEmailPassword(std::string email, int partnercode, std::string password, AuthCallback cb, void *extra, int operation_id, INetPeer *peer) {
		AuthRequest request;
		request.type = EAuthType_User_EmailPassword;
		request.email = email;
		request.partnercode = partnercode;
		request.extra = extra;
		request.callback = cb;
		request.create_session = true;
		request.operation_id = operation_id;
		request.namespaceid = 0;
		if (peer) {
			peer->IncRef();
		}
		request.peer = peer;
		m_auth_task_pool->AddRequest(request);
	}

	void AuthTask::TryAuthTicket(const char *auth_token, const char *server_challenge, const char *client_challenge, const char *response, AuthCallback cb, int operation_id, INetPeer *peer) {
		AuthRequest request;
		request.type = EAuthType_PreAuth_Token;
		request.password = auth_token;
		request.server_challenge = server_challenge;
		request.client_challenge = client_challenge;
		request.client_response = response;
		request.callback = cb;
		request.create_session = true;
		request.operation_id = operation_id;
		request.namespaceid = 0;
		if (peer) {
			peer->IncRef();
		}
		request.peer = peer;
		m_auth_task_pool->AddRequest(request);
	}

	void AuthTask::TryAuthNickEmail_GPHash(std::string nick, std::string email, int partnercode, std::string server_chal, std::string client_chal, std::string client_response, AuthCallback cb, void *extra, int operation_id, INetPeer *peer) {
		AuthRequest request;
		request.type = EAuthType_NickEmail_GPHash;
		request.email = email;
		request.nick = nick;
		request.partnercode = partnercode;
		request.server_challenge = server_chal;
		request.client_challenge = client_chal;
		request.client_response = client_response;
		request.extra = extra;
		request.callback = cb;
		request.create_session = true;
		request.operation_id = operation_id;
		request.namespaceid = 0;
		if (peer) {
			peer->IncRef();
		}
		request.peer = peer;
		m_auth_task_pool->AddRequest(request);
	}
	void AuthTask::TryAuthNickEmail(std::string nick, std::string email, int partnercode, std::string pass, bool make_session, AuthCallback cb, void *extra, int operation_id, INetPeer *peer) {
		AuthRequest request;
		request.type = EAuthType_NickEmail;
		request.email = email;
		request.nick = nick;
		request.partnercode = partnercode;
		request.extra = extra;
		request.callback = cb;
		request.password = pass;
		request.create_session = make_session;
		request.operation_id = operation_id;
		request.namespaceid = 0;
		if (peer) {
			peer->IncRef();
		}
		request.peer = peer;
		m_auth_task_pool->AddRequest(request);
	}
	void AuthTask::TryCreateUser_OrProfile(std::string nick, std::string uniquenick, int namespaceid, std::string email, int partnercode, std::string password, bool create_session, AuthCallback cb, void *extra, int operation_id, INetPeer *peer) {
		AuthRequest request;
		request.type = EAuthType_CreateUser_OrProfile;
		request.email = email;
		request.nick = nick;
		request.uniquenick = uniquenick;
		request.partnercode = partnercode;
		request.namespaceid = namespaceid;
		request.extra = extra;
		request.callback = cb;
		request.password = password;
		request.create_session = create_session;
		request.operation_id = operation_id;
		request.namespaceid = 0;
		if (peer) {
			peer->IncRef();
		}
		request.peer = peer;
		m_auth_task_pool->AddRequest(request);
	}
	void AuthTask::TryAuthPID_GStatsSessKey(int profileid, int session_key, std::string response, AuthCallback cb, void *extra, int operation_id, INetPeer *peer) {
		AuthRequest request;
		request.type = EAuthType_PID_GStats_Sesskey;

		request.profileid = profileid;
		request.session_key = session_key;
		request.client_response = response;

		request.extra = extra;
		request.operation_id = operation_id;
		request.callback = cb;
		request.create_session = true;
		request.namespaceid = 0;
		request.partnercode = 0;
		if (peer) {
			peer->IncRef();
		}
		request.peer = peer;
		m_auth_task_pool->AddRequest(request);
	}
	AuthTask::AuthTask(int thread_index) {
		mp_mutex = OS::CreateMutex();
		mp_thread = OS::CreateThread(AuthTask::TaskThread, this, true);
	}
	AuthTask::~AuthTask() {
		delete mp_mutex;
		delete mp_thread;
	}
	void *AuthTask::TaskThread(CThread *thread) {
		AuthTask *task = (AuthTask *)thread->getParams();
		while (!task->m_request_list.empty() || task->mp_thread_poller->wait()) {
			task->mp_mutex->lock();
			while (!task->m_request_list.empty()) {
				AuthRequest task_params = task->m_request_list.front();
				task->mp_mutex->unlock();
				switch (task_params.type) {
				case EAuthType_NickEmail_GPHash:
					task->PerformAuth_NickEMail_GPHash(task_params);
					break;
				case EAuthType_Uniquenick_Password:
					task->PerformAuth_Uniquenick_Password(task_params);
					break;
				case EAuthType_NickEmail:
					task->PerformAuth_NickEMail(task_params);
					break;
				case EAuthType_PreAuth_Token:
					task->PerformAuth_PreAuth_Token(task_params);
					break;
				case EAuthType_CreateUser_OrProfile:
					task->PerformAuth_CreateUser_OrProfile(task_params);
					break;
				case EAuthType_PID_GStats_Sesskey:
					task->PerformAuth_PID_GSStats_SessKey(task_params);
					break;
				case EAuthType_MakeAuthTicket:
					task->PerformAuth_MakeAuthTicket(task_params);
					break;
				}

				task->mp_mutex->lock();
				if(task_params.peer)
					task_params.peer->DecRef();
				task->m_request_list.pop();
			}

			task->mp_mutex->unlock();
		}
		return NULL;
	}
	void SetupAuthTaskPool(int num_tasks) {
		m_auth_task_pool = new OS::TaskPool<AuthTask, AuthRequest>(num_tasks);
	}
	void ShutdownAuthTaskPool() {
		delete m_auth_task_pool;
	}
}